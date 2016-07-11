#include "rforest.h"

RForest::RForest (Dataset* train_set, TargetData* targdata, MetaData* meta_data, int ntrees, int nvars, bool weights, bool importance, SEXP seeds) {
    /*
     * For training.
     */
    train_set_         = train_set;
    targ_data_         = targdata;
    meta_data_         = meta_data;
    ntrees_            = ntrees;
    mtry_              = nvars;
    weights_           = weights;
    tree_seeds_        = (unsigned int*) INTEGER(seeds);
    nlabels_           = meta_data->nlabels();
    importance_        = importance;
    rf_strength_       = NA_REAL;
    rf_correlation_    = NA_REAL;
    rf_oob_error_rate_ = NA_REAL;
    c_s2_              = NA_REAL;
    emr2_              = NA_REAL;

    tree_vec_    = vector<Tree*>(ntrees);
    bagging_set_ = vector<vector<int> >(ntrees, vector<int>(train_set->nobs(), -1));
    oob_set_vec_ = vector<vector<int> >(ntrees);
}

RForest::RForest (Rcpp::List& wsrf_R, MetaData* meta_data, TargetData* targdata) {
    /*
     * Construct forest from R.
     *
     * For merge, split and prediction.
     */

    importance_        = false;
    tree_seeds_        = NULL;
    rf_strength_       = NA_REAL;
    rf_correlation_    = NA_REAL;
    rf_oob_error_rate_ = NA_REAL;
    c_s2_              = NA_REAL;
    emr2_              = NA_REAL;
    mtry_              = -1;
    weights_           = false;

    train_set_ = NULL;
    targ_data_ = targdata;
    meta_data_ = meta_data;
    nlabels_   = meta_data->nlabels();

//    mtry_       = Rf_isNull(wsrf_R[MTRY_IDX]) ? -2 : Rcpp::as<int>(wsrf_R[MTRY_IDX]);
//    weights_    = Rf_isNull(wsrf_R[WEIGHTS_IDX]) ? false: Rcpp::as<bool>(wsrf_R[WEIGHTS_IDX]);

    vector<vector<vector<double> > > tree_list = Rcpp::as<vector<vector<vector<double> > > >(wsrf_R[TREES_IDX]);
    vector<double> tree_oob_error_rate_vec     = Rcpp::as<vector<double> >(wsrf_R[TREE_OOB_ERROR_RATES_IDX]);

    ntrees_   = tree_list.size();
    tree_vec_ = vector<Tree*>(ntrees_);
    for (int i = 0; i < ntrees_; i++) {
        Tree* tree = new Tree(tree_list[i], meta_data_, tree_oob_error_rate_vec[i]);
        tree_vec_[i] = tree;
    }

    oob_set_vec_ = Rcpp::as<vector<vector<int> > >(wsrf_R[OOB_SETS_IDX]);

    vector<vector<int> >    oob_predict_label_set_vec = Rcpp::as<vector<vector<int> > >(wsrf_R[OOB_PREDICT_LABELS_IDX]);
    vector<vector<double> > tree_IGR_VIs_vec          = Rcpp::as<vector<vector<double> > >(wsrf_R[TREE_IGR_IMPORTANCE_IDX]);
    for (int i = 0; i < ntrees_; i++) {
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
    Tree* decision_tree = new Tree(train_set_, targ_data_, meta_data_, tree_seeds_[ind], &(bagging_set_[ind]), &(oob_set_vec_[ind]));
    decision_tree->build(mtry_, weights_, importance_, pinterrupt);
    tree_vec_[ind] = decision_tree;
}

void RForest::buidForestSeq (volatile bool* pinterrupt) {
    for (int ind = 0; ind < ntrees_; ind++) {
        // check interruption
        if (check_interrupt()) throw interrupt_exception("The random forest model building is interrupted.");

        buildOneTree(ind, pinterrupt);
    }
}

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
/**
 * Build RandomForests::trees_num_ decision trees
 *
 * forest building controller
 *
 * interrupt check points:
 * 1. check at the beginning of every tree building: RandomForests::buildForestAsync()
 * 2. check at the beginning of every node building: DecisionTree::GenerateDecisionTreeByC4_5()
 * 3. check at the beginning of time consuming operation: C4_5AttributeSelectionMethod::HandleDiscreteAttribute() in C4_5AttributeSelectionMethod::ExecuteSelectionByIGR()
 */
void RForest::buildForestAsync (
    int parallel,
    volatile bool* pInterrupt) {

#ifdef WSRF_USE_BOOST
    int nCoresMinusTwo = boost::thread::hardware_concurrency() - 2;
#else
    int nCoresMinusTwo = thread::hardware_concurrency() - 2;
#endif

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
#ifdef WSRF_USE_BOOST
    vector<boost::unique_future<void> > results(nThreads);
    vector<boost::thread> tasks(nThreads);
#else
    vector<future<void> > results(nThreads);
#endif
    this->tree_vec_ = vector<Tree*>(this->ntrees_);
    for (int i = 0; i < nThreads; i++)
#ifdef WSRF_USE_BOOST
    {
        boost::packaged_task<void> pt(boost::bind(&RForest::buildOneTreeAsync, this, &index, pInterrupt));
        results[i] = pt.get_future();
        tasks[i] = boost::thread(boost::move(pt));
    }
#else
    results[i] = async(launch::async, &RForest::buildOneTreeAsync, this, &index, pInterrupt);
#endif

    try {
        bool mark[nThreads];
        for (int i = 0; i < nThreads; i++) mark[i] = false;
        int i = 0;
        do {
            // check each tree builder's status till all are finished
            for (int j = 0; j < nThreads; j++) {
#ifdef WSRF_USE_BOOST
                if (mark[j] != true && results[j].valid() && results[j].wait_for(boost::chrono::seconds (0)) == boost::future_status::ready) {
#else
#if (defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ >= 5))) || defined(__clang__)
                if (mark[j] != true && results[j].valid() && results[j].wait_for(chrono::seconds {0}) == future_status::ready) {
#else
                if (mark[j] != true && results[j].valid() && results[j].wait_for(chrono::seconds {0})) {
#endif
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
#ifdef WSRF_USE_BOOST
        boost::rethrow_exception(boost::current_exception());
#else
        rethrow_exception(current_exception());
#endif
    } // try-catch
}
#endif



#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
/*
 * tree building function: pick a bagging set from bagging_set_ to build one tree a time until no set to fetch
 */
void RForest::buildOneTreeAsync (int* index, volatile bool* pInterrupt)
{
    bool finished = false;
    int ind;

    while (!finished) {

        if (*pInterrupt)
        break;

#ifdef WSRF_USE_BOOST
        boost::unique_lock<boost::mutex> ulk(mut);
#else
        unique_lock<mutex> ulk(mut);
#endif
        if (*index < ntrees_) {
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

#endif

Rcpp::NumericMatrix RForest::predictMatrix (Dataset* data, predictor pred) {

    int nobs    = data->nobs();
    int nlabels = meta_data_->nlabels();
    Rcpp::NumericMatrix res_mat(nlabels, nobs);
    double* iter = REAL(SEXP(res_mat));

    for (int i = 0; i < nobs; ++i) {
        (this->*pred)(data, i, iter);
        iter += nlabels;
    }

    Rcpp::List dimnames;
    dimnames.push_back(meta_data_->getLabelNames());
    res_mat.attr("dimnames") = dimnames;

    return res_mat;

}

Rcpp::IntegerVector RForest::predictClassVec (Dataset* data) {
    int nobs    = data->nobs();
    int nlabels = meta_data_->nlabels();

    Rcpp::NumericMatrix res_mat = predictMatrix(data, &RForest::predictLabelFreqCount);
    Rcpp::IntegerVector res_vec(nobs);

    double* iter = REAL(SEXP(res_mat));
    for (int i = 0; i < nobs; iter += nlabels, i++)
        res_vec[i] = distance(iter, max_element(iter, iter+nlabels))+1;

    res_vec.attr("levels") = meta_data_->getLabelNames();
    res_vec.attr("class") = "factor";
    return res_vec;
}

void RForest::collectBasicStatistics () {

    int nvars = meta_data_->nvars();
    for (int treeidx = 0; treeidx < ntrees_; ++treeidx) {
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
        IGR_VIs_[vindex] /= ntrees_;
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
    for (int treeidx = 0; treeidx < ntrees_; ++treeidx) {

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

    double esd = sum_sd / ntrees_;

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
    for (int tindex = 0; tindex < ntrees_; tindex++) {
        vector<double>& label_VIs = tree_vec_[tindex]->getTreePermVIs();

        for (int i = 0; i < size; i++)
            raw_perm_VIs_[i] += label_VIs[i];
    }

    for (int i = 0; i < size; i++)
        raw_perm_VIs_[i] /= ntrees_;

    /*
     * Calculate standard deviation.
     */
    for (int tindex = 0; tindex < ntrees_; tindex++) {
        vector<double>& label_VIs = tree_vec_[tindex]->getTreePermVIs();

        for (int i = 0; i < size; i++) {
            double diff = label_VIs[i] - raw_perm_VIs_[i];
            sigma_perm_VIs_[i] += diff * diff;
        }
    }

    for (int i = 0; i < size; i++)
        sigma_perm_VIs_[i] = sqrt(sigma_perm_VIs_[i]) / ntrees_;

}

void RForest::saveModel (Rcpp::List& wsrf_R) {
    /*
     * Save the model.  Only used for a model of wsrf, not for combine or merge.
     */

    wsrf_R[WEIGHTS_IDX]        = Rcpp::wrap(weights_);
    wsrf_R[MTRY_IDX]           = Rcpp::wrap(mtry_);

    vector<vector<vector<double> > > trees(ntrees_);
    vector<double> tree_oob_error_rates(ntrees_);
    for (int i = 0; i < ntrees_; i++) {
        tree_vec_[i]->save(trees[i]);
        tree_oob_error_rates[i] = tree_vec_[i]->getTreeOOBErrorRate();
    }

    wsrf_R[TREES_IDX]                = Rcpp::wrap(trees);
    wsrf_R[TREE_OOB_ERROR_RATES_IDX] = Rcpp::wrap(tree_oob_error_rates);
    wsrf_R[OOB_SETS_IDX]             = Rcpp::wrap(oob_set_vec_);

    vector<vector<int> > oob_predict_label_set_vec(ntrees_);
    vector<vector<double> > tree_IGR_VIs_vec(ntrees_);
    for (int i = 0; i < ntrees_; i++) {
        oob_predict_label_set_vec[i].swap(tree_vec_[i]->getOOBPredictLabelSet());
        tree_IGR_VIs_vec[i].swap(tree_vec_[i]->getTreeIGRVIs());
    }
    wsrf_R[OOB_PREDICT_LABELS_IDX]  = Rcpp::wrap(oob_predict_label_set_vec);
    wsrf_R[TREE_IGR_IMPORTANCE_IDX] = Rcpp::wrap(tree_IGR_VIs_vec);
}

void RForest::saveMeasures (Rcpp::List& wsrf_R) {
    /*
     * Calculated values, need not to be unserialized.
     *
     * And all related data should be initialized in RForest::calcEvalMeasures();
     */

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
