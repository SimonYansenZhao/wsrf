#include "rforest.h"

RForest::RForest (
        Dataset* train_set,
        TargetData* targdata,
        MetaData* meta_data,
        int ntree,
        int nvars,
        int min_node_size,
        bool weights,
        bool importance,
        SEXP seeds)
/*
 * For training.
 */
{

    train_set_         = train_set;
    targ_data_         = targdata;
    meta_data_         = meta_data;
    ntree_            = ntree;
    mtry_              = nvars;
    min_node_size_     = min_node_size;
    weights_           = weights;
    tree_seeds_        = (unsigned int*) INTEGER(seeds);
    nlabels_           = meta_data->nlabels();
    importance_        = importance;
    rf_strength_       = NA_REAL;
    rf_correlation_    = NA_REAL;
    rf_oob_error_rate_ = NA_REAL;
    c_s2_              = NA_REAL;
    emr2_              = NA_REAL;

    tree_vec_    = vector<Tree*>(ntree);
    bagging_set_ = vector<vector<int> >(ntree, vector<int>(train_set->nobs(), -1));
    oob_set_vec_ = vector<vector<int> >(ntree);

    if (mtry_ == -1) mtry_ = log((double)(meta_data_->nvars()))/LN_2 + 1;
}

RForest::RForest (Rcpp::List& wsrf_R, MetaData* meta_data, TargetData* targdata)
/*
 * Construct forest from R.
 *
 * For merge, split and prediction.
 */
{
    importance_        = false;
    tree_seeds_        = NULL;
    rf_strength_       = NA_REAL;
    rf_correlation_    = NA_REAL;
    rf_oob_error_rate_ = NA_REAL;
    c_s2_              = NA_REAL;
    emr2_              = NA_REAL;
    mtry_              = -1;
    weights_           = false;
    min_node_size_     = 2;

    train_set_ = NULL;
    targ_data_ = targdata;
    meta_data_ = meta_data;
    nlabels_   = meta_data->nlabels();

//    mtry_       = Rf_isNull(wsrf_R[MTRY_IDX]) ? -2 : Rcpp::as<int>(wsrf_R[MTRY_IDX]);
//    weights_    = Rf_isNull(wsrf_R[WEIGHTS_IDX]) ? false: Rcpp::as<bool>(wsrf_R[WEIGHTS_IDX]);

    vector<vector<vector<double> > > tree_list = Rcpp::as<vector<vector<vector<double> > > >((SEXPREC*)wsrf_R[TREES_IDX]);
    vector<double> tree_oob_error_rate_vec     = Rcpp::as<vector<double> >((SEXPREC*)wsrf_R[TREE_OOB_ERROR_RATES_IDX]);

    ntree_   = tree_list.size();
    tree_vec_ = vector<Tree*>(ntree_);
    for (int i = 0; i < ntree_; i++) {
        Tree* tree = new Tree(tree_list[i], meta_data_, tree_oob_error_rate_vec[i]);
        tree_vec_[i] = tree;
    }

    oob_set_vec_ = Rcpp::as<vector<vector<int> > >((SEXPREC*)wsrf_R[OOB_SETS_IDX]);

    vector<vector<int> >    oob_predict_label_set_vec = Rcpp::as<vector<vector<int> > >((SEXPREC*)wsrf_R[OOB_PREDICT_LABELS_IDX]);
    vector<vector<double> > tree_IGR_VIs_vec          = Rcpp::as<vector<vector<double> > >((SEXPREC*)wsrf_R[TREE_IGR_IMPORTANCE_IDX]);
    for (int i = 0; i < ntree_; i++) {
        tree_vec_[i]->setOOBPredictLabelSet(oob_predict_label_set_vec[i]);
        tree_vec_[i]->setTreeIGRVIs(tree_IGR_VIs_vec[i]);
    }

//    Rcpp::NumericVector eval = model_list[EVALUATION];
//    rf_oob_error_rate_ = eval[RF_OOB_ERROR_RATE];
//    rf_strength_       = eval[STRENGTH];
//    rf_correlation_    = eval[CORRELATION];
//    c_s2_              = eval[C_S2];

}

RForest::~RForest () {
    int n = tree_vec_.size();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            Tree* tree = tree_vec_[i];
            if (tree) delete tree;
        }
    }
}

void RForest::buildOneTree (int ind, volatile bool* pinterrupt) {
    Tree* decision_tree = new Tree(
            train_set_,
            targ_data_,
            meta_data_,
            min_node_size_,
            tree_seeds_[ind],
            &(bagging_set_[ind]),
            &(oob_set_vec_[ind]),
            mtry_,
            weights_,
            importance_);
    decision_tree->build(pinterrupt);
    tree_vec_[ind] = decision_tree;
}

void RForest::buidForestSeq (volatile bool* pinterrupt) {
    for (int ind = 0; ind < ntree_; ind++) {
        // check interruption
        if (check_interrupt()) throw interrupt_exception(INTERRUPT_MSG);

        buildOneTree(ind, pinterrupt);
    }
}

void RForest::buildForestAsync (
    int parallel,
    volatile bool* pInterrupt)
/*
 * Build RandomForests::trees_num_ decision trees
 *
 * forest building controller
 *
 * interrupt check points:
 * 1. check at the beginning of every tree building: RandomForests::buildForestAsync()
 * 2. check at the beginning of every node building: DecisionTree::GenerateDecisionTreeByC4_5()
 * 3. check at the beginning of time consuming operation: C4_5AttributeSelectionMethod::HandleDiscreteAttribute() in C4_5AttributeSelectionMethod::ExecuteSelectionByIGR()
 */
{

    int nCoresMinusTwo = thread::hardware_concurrency() - 2;

    // simultaneously build <nThreads> trees until <tree_num_> trees has been built
    int nThreads;
    if (parallel < 0) {
        // gcc 4.6 do not support std::thread::hardware_concurrency(), we fix the thread number to 10
        nThreads = nCoresMinusTwo > 1 ? nCoresMinusTwo : 10;
    } else {
        nThreads = parallel;
    }

    // using <nThreads> tree builder to build trees
    int index = 0;
    vector<future<void> > results(nThreads);
    this->tree_vec_ = vector<Tree*>(this->ntree_);
    for (int i = 0; i < nThreads; i++)
        results[i] = async(launch::async, &RForest::buildOneTreeAsync, this, &index, pInterrupt);

    try {
        vector<bool> mark(nThreads, false);
        int i = 0;
        do {
            // check each tree builder's status till all are finished
            for (int j = 0; j < nThreads; j++) {
#if (defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ >= 5))) || defined(__clang__)
                if (mark[j] != true && results[j].valid() && results[j].wait_for(chrono::seconds {0}) == future_status::ready) {
#else
                if (mark[j] != true && results[j].valid() && results[j].wait_for(chrono::seconds {0})) {
#endif
                    results[j].get();
                    mark[j] = true;
                    i++;
                } // if ()
            } // for ()
//    this_thread::sleep_for(chrono::seconds(1));
        } while (i < nThreads);
    } catch (...) {
        (*pInterrupt) = true;  // if one tree builder throw a exception, set true to inform others
        rethrow_exception(current_exception());
    } // try-catch
}

void RForest::buildOneTreeAsync (int* index, volatile bool* pInterrupt)
/*
 * tree building function: pick a bagging set from bagging_set_ to build one tree a time until no set to fetch
 */
{
    bool finished = false;
    int ind;

    while (!finished) {

        if (*pInterrupt)
        break;

        unique_lock<mutex> ulk(mut_);
        if (*index < ntree_) {
            ind = *index;
            (*index)++;
            ulk.unlock();
        } else {
            finished = true;
        }

        if (!finished) {
            buildOneTree(ind, pInterrupt);
        }
    }
}

Rcpp::List RForest::predict (Dataset* data, int type) {
    // 0 - class; 1 - vote; 2 - prob; 3 - aprob; 4 - waprob
    Rcpp::List res(PRED_TYPE_NUM);
    double* res_iter[PRED_TYPE_NUM];
    int* class_iter;

    int nobs = data->nobs();

    bool need_class  = type & PRED_TYPE_CLASS;
    bool need_vote   = type & PRED_TYPE_VOTE;
    bool need_prob   = type & PRED_TYPE_PROB;
    bool need_aprob  = type & PRED_TYPE_APROB;
    bool need_waprob = type & PRED_TYPE_WAPROB;

    // Allocate memory.
    for (int tindex = 0, left_type = type; tindex < PRED_TYPE_NUM; tindex++, left_type >>= 1) {
        if (left_type % 2                                     // For required prediction types.
            || ( tindex == PRED_TYPE_VOTE_IDX && need_class)  // For vote if class is required.
            ) {

            if (tindex == PRED_TYPE_CLASS_IDX) {  // class or response
                Rcpp::IntegerVector temp(nobs);
                temp.attr("levels") = meta_data_->getLabelNames();
                temp.attr("class") = "factor";
                res[tindex] = temp;
                class_iter = INTEGER(SEXP(temp));
            } else {  // vote, prob, aprob or waprob
                Rcpp::NumericMatrix temp(nlabels_, nobs);
                Rcpp::List dimnames;
                dimnames.push_back(meta_data_->getLabelNames());
                temp.attr("dimnames") = dimnames;
                res[tindex] = temp;
                res_iter[tindex] = REAL(SEXP(temp));
            }

        } else
            res[tindex] = R_NilValue;
    }

    // Get predictions.
    for (int obs_idx = 0; obs_idx < nobs; ++obs_idx) {

        if ((obs_idx & 0x3ff) == 0 && check_interrupt()) throw interrupt_exception(INTERRUPT_MSG);

        double sumAccuracy = 0;

        // Get leaf node information.
        for (vector<Tree*>::iterator iter = tree_vec_.begin(); iter != tree_vec_.end(); ++iter) {
            Node* node = (*iter)->predictLeafNode(data, obs_idx);

            if (need_vote || need_class) res_iter[PRED_TYPE_VOTE_IDX][node->label()]++;  // vote or class

            if (need_prob) res_iter[PRED_TYPE_PROB_IDX][node->label()]++;  // prob

            if (need_aprob) {  // aprob
                vector<double> classDistributions = node->getLabelDstr();
                for (int lab_idx = 0; lab_idx < nlabels_; lab_idx++)
                    res_iter[PRED_TYPE_APROB_IDX][lab_idx] += classDistributions[lab_idx];
            }

            if (need_waprob) {  // waprob
                vector<double> classDistributions = node->getLabelDstr();
                double accuracy = 1 - (*iter)->getTreeOOBErrorRate();
                sumAccuracy += accuracy;
                for (int lab_idx = 0; lab_idx < nlabels_; lab_idx++)
                    res_iter[PRED_TYPE_WAPROB_IDX][lab_idx] += classDistributions[lab_idx] * accuracy;
            }
        }

        // Calculate predictions.
        if (need_class) {  // class or response
            class_iter[obs_idx] =
                distance(res_iter[PRED_TYPE_VOTE_IDX],
                         max_element(res_iter[PRED_TYPE_VOTE_IDX],
                                     res_iter[PRED_TYPE_VOTE_IDX] + nlabels_))
                 + 1;
        }

        if (need_prob) {  // prob
            for (int lab_idx = 0; lab_idx < nlabels_; lab_idx++)
                res_iter[PRED_TYPE_PROB_IDX][lab_idx] /= ntree_;
        }

        if (need_aprob) {  // aprob
            for (int lab_idx = 0; lab_idx < nlabels_; lab_idx++)
                res_iter[PRED_TYPE_APROB_IDX][lab_idx] /= ntree_;
        }

        if (need_waprob) {  // waprob
            for (int lab_idx = 0; lab_idx < nlabels_; lab_idx++)
                res_iter[PRED_TYPE_WAPROB_IDX][lab_idx] /= sumAccuracy;
        }

        // Update pointers.
        for (int tindex = 1, left_type = type >> 1; tindex < PRED_TYPE_NUM; tindex++, left_type >>= 1) {
            if (left_type % 2 || ( tindex == PRED_TYPE_VOTE_IDX && need_class))
                res_iter[tindex] += nlabels_;
        }
    }

    // Remove vote because it is used for class.
    if (need_class && !need_vote)
        res[PRED_TYPE_VOTE_IDX] = R_NilValue;

    // Transpose matrix.
    for (int tindex = 1, left_type = type >> 1; tindex < PRED_TYPE_NUM; tindex++, left_type >>= 1) {
        if (left_type % 2) res[tindex] = Rcpp::transpose(Rcpp::as<Rcpp::NumericMatrix>((SEXPREC*)res[tindex]));
    }

    return res;
}

void RForest::collectBasicStatistics () {

    int nvars = meta_data_->nvars();
    for (int treeidx = 0; treeidx < ntree_; ++treeidx) {
        vector<int>& poob_vec = oob_set_vec_[treeidx];
        vector<int>& oob_predict_label_set = tree_vec_[treeidx]->getOOBPredictLabelSet();
        int n = poob_vec.size();

        for (int i = 0; i < n; i++) {
            int obsidx = poob_vec[i];

            oob_predict_label_freq_matrix_[obsidx][oob_predict_label_set[i]]++;
            oob_count_vec_[obsidx]++;
        }

        // calculate IGR decreases for each variable.
        vector<double>& tree_IGR_VIs = tree_vec_[treeidx]->getTreeIGRVIs();
        for (int vindex = 0; vindex < nvars; vindex++)
            IGR_VIs_[vindex] += tree_IGR_VIs[vindex];
    }

    for (int vindex = 0; vindex < nvars; vindex++)
        IGR_VIs_[vindex] /= ntree_;
}

void RForest::calcOOBConfusionErrorRateAndStrength () {
    int nobs = targ_data_->nobs();
    int oob_num = 0;
    int error_num = 0;
    double sum_mr = 0.0;
    double sum_mr2 = 0.0;
    int class_err_idx = nlabels_*nlabels_;

    for (int obsidx = 0; obsidx < nobs; ++obsidx) {
        if (oob_count_vec_[obsidx] != 0) {
            oob_num++;
            vector<int>& numbers = oob_predict_label_freq_matrix_[obsidx];

            oob_predict_label_vec_[obsidx] = distance(numbers.begin(), max_element(numbers.begin(), numbers.end()));

            int actual_label = targ_data_->getLabel(obsidx) - 1;
            int predict_label = oob_predict_label_vec_[obsidx];

            // For confusion matrix.
            if (predict_label == actual_label) {
                oob_confusion_matrix_[actual_label*nlabels_ + actual_label]++;
            } else {
                oob_confusion_matrix_[predict_label*nlabels_ + actual_label]++;
                error_num++;
            }
            oob_confusion_matrix_[class_err_idx + actual_label]++;

            // For strength.
            int max_j = -1;
            for (int label = 0, max_num = -1; label < nlabels_; ++label)
                if (label != actual_label && numbers[label] > max_num) {
                    max_j = label;
                    max_num = numbers[label];
                }

            max_j_[obsidx] = max_j;
            double mr = (numbers[actual_label] - numbers[max_j]) / (double) (oob_count_vec_[obsidx]);
            sum_mr += mr;
            sum_mr2 += mr * mr;
        }
    }

    for (int i = 0; i < nlabels_; i++)
        oob_confusion_matrix_[class_err_idx + i] = 1 - oob_confusion_matrix_[i*nlabels_ + i]/oob_confusion_matrix_[class_err_idx + i];

    rf_oob_error_rate_ = error_num / (double) oob_num;
    emr2_ = sum_mr2 / oob_num;
    rf_strength_ = sum_mr / oob_num;
}

void RForest::calcRFCorrelationAndCS2 () {
    double sum_sd = 0.0;
    for (int treeidx = 0; treeidx < ntree_; ++treeidx) {

        vector<int>& poob_vec = oob_set_vec_[treeidx];
        vector<int>& oob_predict_label_set = tree_vec_[treeidx]->getOOBPredictLabelSet();
        int n = poob_vec.size();
        int count = 0;

        for (int i = 0; i < n; i++)
            if (oob_predict_label_set[i] == max_j_[poob_vec[i]])
                count++;

        double p1 = 1 - tree_vec_[treeidx]->getTreeOOBErrorRate();
        double p2 = count / (double) n;

        double base = p1 + p2 + (p1 - p2) * (p1 - p2);
        sum_sd += sqrt(base);
    }

    double esd = sum_sd / ntree_;

    rf_correlation_ = (emr2_ - rf_strength_ * rf_strength_) / (esd * esd);
    c_s2_           = rf_correlation_ / (rf_strength_ * rf_strength_);
}

void RForest::assessPermVariableImportance () {

    int size = (nlabels_ + 1) * meta_data_->nvars();
    raw_perm_VIs_   = vector<double>(size, 0);
    sigma_perm_VIs_ = vector<double>(size, 0);

    /*
     * Calculate raw variable importance.
     */
    for (int tindex = 0; tindex < ntree_; tindex++) {
        vector<double>& label_VIs = tree_vec_[tindex]->getTreePermVIs();

        for (int i = 0; i < size; i++)
            raw_perm_VIs_[i] += label_VIs[i];
    }

    for (int i = 0; i < size; i++)
        raw_perm_VIs_[i] /= ntree_;

    /*
     * Calculate standard deviation.
     */
    for (int tindex = 0; tindex < ntree_; tindex++) {
        vector<double>& label_VIs = tree_vec_[tindex]->getTreePermVIs();

        for (int i = 0; i < size; i++) {
            double diff = label_VIs[i] - raw_perm_VIs_[i];
            sigma_perm_VIs_[i] += diff * diff;
        }
    }

    for (int i = 0; i < size; i++)
        sigma_perm_VIs_[i] = sqrt(sigma_perm_VIs_[i]) / ntree_;

}

void RForest::saveModel (Rcpp::List& wsrf_R)
/*
 * Save the model.  Only used for a model of wsrf, not for combine or merge.
 */
{
    wsrf_R[WEIGHTS_IDX]  = Rcpp::wrap(weights_);
    wsrf_R[MTRY_IDX]     = Rcpp::wrap(mtry_);
    wsrf_R[NODESIZE_IDX] = Rcpp::wrap(min_node_size_);

    vector<vector<vector<double> > > trees(ntree_);
    vector<double> tree_oob_error_rates(ntree_);
    for (int i = 0; i < ntree_; i++) {
        tree_vec_[i]->save(trees[i]);
        tree_oob_error_rates[i] = tree_vec_[i]->getTreeOOBErrorRate();
    }

    wsrf_R[TREES_IDX]                = Rcpp::wrap(trees);
    wsrf_R[TREE_OOB_ERROR_RATES_IDX] = Rcpp::wrap(tree_oob_error_rates);
    wsrf_R[OOB_SETS_IDX]             = Rcpp::wrap(oob_set_vec_);

    vector<vector<int> > oob_predict_label_set_vec(ntree_);
    vector<vector<double> > tree_IGR_VIs_vec(ntree_);
    for (int i = 0; i < ntree_; i++) {
        oob_predict_label_set_vec[i].swap(tree_vec_[i]->getOOBPredictLabelSet());
        tree_IGR_VIs_vec[i].swap(tree_vec_[i]->getTreeIGRVIs());
    }
    wsrf_R[OOB_PREDICT_LABELS_IDX]  = Rcpp::wrap(oob_predict_label_set_vec);
    wsrf_R[TREE_IGR_IMPORTANCE_IDX] = Rcpp::wrap(tree_IGR_VIs_vec);
}

void RForest::saveMeasures (Rcpp::List& wsrf_R)
/*
 * Calculated values, need not to be unserialized.
 *
 * And all related data should be initialized in RForest::calcEvalMeasures();
 */
{
    wsrf_R[RF_OOB_ERROR_RATE_IDX] = Rcpp::wrap(rf_oob_error_rate_);
    wsrf_R[STRENGTH_IDX]          = Rcpp::wrap(rf_strength_);
    wsrf_R[CORRELATION_IDX]       = Rcpp::wrap(rf_correlation_);
    wsrf_R[C_S2_IDX]              = Rcpp::wrap(c_s2_);

    vector<string> labelnames = meta_data_->getLabelNames();

    int n = oob_predict_label_vec_.size();
    for (int i = 0; i < n; i++)
        oob_predict_label_vec_[i]++;  // R factor levels starts with 1.
    Rcpp::IntegerVector predict_vec = Rcpp::wrap(oob_predict_label_vec_);
    predict_vec.attr("levels") = Rcpp::wrap(labelnames);
    predict_vec.attr("class") = "factor";
    wsrf_R[PREDICTED_IDX] = predict_vec;
    wsrf_R[OOB_TIMES_IDX] = Rcpp::wrap(oob_count_vec_);

    // R matrix is column-wise, while C matrix is row-wise.
    Rcpp::NumericMatrix confusion_mat(nlabels_, nlabels_+1, oob_confusion_matrix_.begin());
    Rcpp::List dimnames;
    dimnames.push_back(labelnames);
    labelnames.push_back("class.error");
    dimnames.push_back(labelnames);
    confusion_mat.attr("dimnames") = dimnames;
    wsrf_R[CONFUSION_IDX] = confusion_mat;

    Rcpp::NumericMatrix importance_mat;
    Rcpp::NumericMatrix::iterator iter;

    if (importance_) {
        importance_mat = Rcpp::NumericMatrix(meta_data_->nvars(), nlabels_+2);
        iter = copy(raw_perm_VIs_.begin(), raw_perm_VIs_.end(), importance_mat.begin());
        copy(IGR_VIs_.begin(), IGR_VIs_.end(), iter);

        Rcpp::List dimnames;
        dimnames.push_back(meta_data_->getVarNames());
        labelnames.back() = "MeanDecreaseAccuracy";
        labelnames.push_back("MeanDecreaseIGR");
        dimnames.push_back(labelnames);
        importance_mat.attr("dimnames") = dimnames;
        wsrf_R[IMPORTANCE_IDX] = importance_mat;

        Rcpp::NumericMatrix importancesd_mat = Rcpp::NumericMatrix(meta_data_->nvars(), nlabels_+1, sigma_perm_VIs_.begin());
        Rcpp::List dimnames2 = Rcpp::clone(dimnames);
        dimnames2.erase(1);
        labelnames.pop_back();
        dimnames2.push_back(labelnames);
        importancesd_mat.attr("dimnames") = dimnames2;
        wsrf_R[IMPORTANCESD_IDX] = importancesd_mat;
    } else {
        importance_mat = Rcpp::NumericMatrix(meta_data_->nvars(), 1, IGR_VIs_.begin());

        Rcpp::List dimnames;
        dimnames.push_back(meta_data_->getVarNames());
        dimnames.push_back("MeanDecreaseIGR");
        importance_mat.attr("dimnames") = dimnames;
        wsrf_R[IMPORTANCE_IDX] = importance_mat;
    }

}
