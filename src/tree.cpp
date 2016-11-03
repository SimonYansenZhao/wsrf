#include "tree.h"

Tree::Tree (Dataset* train_set,
        TargetData* targdata,
        MetaData* meta_data,
        int min_node_size,
        unsigned seed,
        vector<int>* pbagging_vec,
        vector<int>* poob_vec,
        int mtry,
        bool isweight,
        bool isimportance,
        volatile bool* pInterrupt,
        bool isParallel) {

    train_set_     = train_set;
    targ_data_     = targdata;
    meta_data_     = meta_data;
    min_node_size_ = min_node_size;
    seed_          = seed;
    pbagging_vec_  = pbagging_vec;
    poob_vec_      = poob_vec;
    nnodes_        = 0;
    node_id_       = 0;
    root_          = NULL;
    mtry_          = mtry;
    isweight_      = isweight;
    isimportance_  = isimportance;

    tree_oob_error_rate_  = NA_REAL;
    label_oob_error_rate_ = vector<double>(meta_data->nlabels(), 0);
    tree_IGR_VIs_         = vector<double>(meta_data->nvars(), 0);

    pInterrupt_ = pInterrupt;
    isParallel_ = isParallel;

    resetPerm(true);
}

Tree::Tree (const vector<vector<double> >& node_infos, MetaData* meta_data, double tree_oob_error_rate)
/*
 * Reconstruct the tree from <node_infos>.
 *
 * See Tree::save() for details.
 */
{
    train_set_    = NULL;
    targ_data_    = NULL;
    meta_data_    = meta_data;
    node_id_      = 0;
    poob_vec_     = NULL;
    pbagging_vec_ = NULL;
    seed_         = NA_INTEGER;

    pInterrupt_ = NULL;
    isParallel_ = false;

    tree_oob_error_rate_ = tree_oob_error_rate;

    queue<Node*> noparent_nodes;
    nnodes_ = node_infos.size();

    for (int i = nnodes_ - 1; i >= 0; i--) {
        Node* node = new Node(node_infos[i], meta_data_);
        noparent_nodes.push(node);

        for (int j = node->nchild() - 1; j >= 0; j--) {
            node->setChild(j, noparent_nodes.front());
            noparent_nodes.pop();
        }
    }

    root_ = noparent_nodes.front();

    resetPerm(true);
}

void Tree::genBaggingSets ()
/*
 * Sample the observations with replacement.
 * Generate bagging data set and out-of-bag data set.
 */
{

    //TODO: If possible, make similar RNG codes into a single function.

    int nobs = train_set_->nobs();
    vector<bool> selected_status(nobs, false);

    uniform_int_distribution<int> uid {0, nobs - 1};
    default_random_engine re {seed_};

    for (int j = 0; j < nobs; ++j) {
        int random_num = uid(re);

        (*pbagging_vec_)[j] = random_num;
        selected_status[random_num] = true;
    }

    vector<int> oob;
    for (int ind = 0; ind < nobs; ind++)
        if (!selected_status[ind]) oob.push_back(ind);
    poob_vec_->swap(oob);

    oob_predict_label_set_ = vector<int>(poob_vec_->size());
}

void Tree::build ()
/*
 * Grow a tree.
 */
{
    genBaggingSets();
    root_ = genC4p5Tree(*pbagging_vec_, meta_data_->getFeatureVars());

    if (!isParallel_ && check_interrupt()) {
        // If run sequentially, check user interruption directly.
        throw interrupt_exception(MODEL_INTERRUPT_MSG);
    } else if (*pInterrupt_) {
        // If interrupted in parallel, tree nodes would be NULL, subsequent calculation would not proceed.
        return;
    }

    calcOOBMeasures(isimportance_);
}

Node* Tree::genC4p5Tree (const vector<int>& obs_vec, const vector<int>& var_vec)
/*
 * Build a tree recursively.
 */
{

    if (!isParallel_ && check_interrupt()) {
        // If run sequentially, check user interruption directly.
        throw interrupt_exception(MODEL_INTERRUPT_MSG);
    } else if (*pInterrupt_ ) {
        // Otherwise, return immediately if run in parallel.
        return NULL;
    }

    int nobs = obs_vec.size();
    if (targ_data_->haveSameLabel(obs_vec)) {
        // All observations have the same class label
        return createLeafNode(obs_vec, nobs, true);

    } else if (var_vec.size() == 0) {
        // No variables left for split
        return createLeafNode(obs_vec, nobs, false);

    } else {
        VarSelectRes result;
        if (isweight_) {
            C4p5Selector method(train_set_, targ_data_, meta_data_, min_node_size_, obs_vec, var_vec, mtry_, seed_, pInterrupt_, isParallel_);
            method.doIGRSelection(result);
        } else {
            C4p5Selector method(train_set_, targ_data_, meta_data_, min_node_size_, obs_vec, var_vec, mtry_, seed_, pInterrupt_, isParallel_);
            method.doSelection(result);
        }

        seed_ += 1;  // Change seed.

        if (!result.ok_) {
            // If no better attribute selected
            return createLeafNode(obs_vec, nobs, false);

        } else {
            Node* node;

            if (meta_data_->getVarType(result.var_idx_) == DISCRETE) {
                node = createInternalNode(nobs, result);
                vector<int> new_var_vec = removeOneVar(var_vec, result.var_idx_);
                for (map<int, vector<int> >::iterator iter = result.split_map_.begin(); iter != result.split_map_.end(); ++iter) {
                    if (iter->second.size() == 0) {
                        // Use parent node statistics
                        node->setChild(iter->first, createLeafNode(obs_vec, 0, false));
                    } else {
                        node->setChild(iter->first, genC4p5Tree(iter->second, new_var_vec));
                    }
                }
            } else {
                node = createInternalNode(nobs, result);
                node->setSplitValue(result.split_value_);
                for (map<int, vector<int> >::iterator iter = result.split_map_.begin(); iter != result.split_map_.end(); ++iter)
                    node->setChild(iter->first, genC4p5Tree(iter->second, var_vec));
            }

            return node;
        }
    }
}

template<class T>
void Tree::copyPermData ()
/*
 * Copy data from training set to perm_var_data_, to prepare for permutation.
 */
{

    //TODO: Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.

    T*  var_array = train_set_->getVar <T> (perm_var_idx_);

    copy(var_array, var_array + train_set_->nobs(), perm_var_data_.begin());
}

void Tree::permute (int index)
/*
 * Permute the values of variable <index> for preparation of assessing variable importance.
 */
{
    perm_var_idx_ = index;

    switch (meta_data_->getVarType(perm_var_idx_)) {
    case DISCRETE:
    case INTSXP:
        copyPermData<int>();
        break;
    case REALSXP:
        copyPermData<double>();
        break;
    }

    // Do permutation.

    //TODO: If possible, make similar RNG codes into a single function.

    default_random_engine re {seed_};

    for (int i = train_set_->nobs()-1; i > 0; --i) {

        uniform_int_distribution<int> uid {0, i};
        int random_num = uid(re);

        swap(perm_var_data_[i], perm_var_data_[random_num]);

    }
}

void Tree::resetPerm (bool initial) {
    perm_var_idx_ = -1;
    if (!initial) {
        if (perm_var_data_.size() != 0) perm_var_data_ = vector<double>();
        if (perm_is_var_used_.size() != 0) perm_is_var_used_ = vector<bool>();
    }
}

void Tree::calcOOBMeasures (bool importance)
/*
 * return error rate for classifying training set
 */
{
    // TODO: Extract a method for OOB error rate.

    int error_oob = 0;
    int nobs_oob  = poob_vec_->size();
    int nlabels   = meta_data_->nlabels();

    vector<double> labelcounts(nlabels, 0);

    for (int i = 0; i < nobs_oob; i++) {
        int index         = (*poob_vec_)[i];
        int predict_label = predictLabel(train_set_, index);
        int actual_label  = targ_data_->getLabel(index) - 1;

        oob_predict_label_set_[i] = predict_label;
        labelcounts[actual_label]++;

        if (predict_label != actual_label) {
            error_oob++;
            label_oob_error_rate_[actual_label]++;
        }
    }

    tree_oob_error_rate_ = error_oob / (double) nobs_oob;

    for (int i = 0; i < nlabels; i++)
        label_oob_error_rate_[i] /= labelcounts[i];


    // Assess variable importance.
    if (importance) {
        int nvars = meta_data_->nvars();

        perm_is_var_used_ = vector<bool>(nvars, false);
        perm_var_data_    = vector<double>(train_set_->nobs());
        tree_perm_VIs_    = vector<double>((nlabels+1) * nvars, 0);
        vector<int> row_indexes (nlabels+1);  // Mark the index of each row for matrix of size (nlabels+1) * nvars.
        for (int i = 0, row = 0; i < nlabels+1; row += nvars, i++)
            row_indexes[i] = row;

        // Find used variables and mark in perm_is_var_used_.
        doSthOnNodes(root_, &Tree::markOneVarUsed);

        /*
         * Calculate percent increase in mis-classification rate.
         * TODO: Here we permute only once for each variable used for node splitting.
         *       We may let the user to specify the number of times for permutation.
         */
        for (int var_idx = 0; var_idx < nvars; var_idx++) {
            if (perm_is_var_used_[var_idx]) {
                int total_error_oob = 0;
                permute(var_idx);
                for (int i = 0; i < nobs_oob; i++) {
                    int index = (*poob_vec_)[i];
                    int predict_label = predictLabel(train_set_, index);
                    int actual_label  = targ_data_->getLabel(index) - 1;

                    if (predict_label != actual_label) {
                        total_error_oob++;
                        tree_perm_VIs_[row_indexes[actual_label] + var_idx]++;
                    }
                }

                // Percent increase in total out-of-bag error rate after permutation.
                tree_perm_VIs_[row_indexes[nlabels] + var_idx] = total_error_oob / (double) nobs_oob - tree_oob_error_rate_;

                // Percent increase in each label out-of-bag error rate after permutation.
                for (int i = 0; i < nlabels; i++)
                    tree_perm_VIs_[row_indexes[i] + var_idx] = tree_perm_VIs_[row_indexes[i] + var_idx] / labelcounts[i] - label_oob_error_rate_[i];
            }
        }

        // Release temporary memory.
        resetPerm(false);
    }
}

void Tree::printTree (Node* node, int level) {

    if (node->type() == INTERNALNODE) {
        int id = ++node_id_;

        string indent = "";
        for (int i = 0; i < level; i++)
            indent += " ..";

        int    varidx  = node->getVarIdx();
        int    nchild  = node->nchild();
        string varname = meta_data_->getVarName(varidx);
        int    vartype = meta_data_->getVarType(varidx);

        if (vartype == DISCRETE) {
            for (int i = 0; i < nchild; i++) {
                string value = meta_data_->getValueName(varidx, i);
                printNodeInfo("%s %d) %s == %s", indent, id, varname, value.c_str(), node->getChild(i));
                printTree(node->getChild(i), level + 1);
            }
        } else {
            char numstr[21]; // enough to hold all numbers up to 64-bits
            sprintf(numstr, "%.10g", node->getSplitValue());
            printNodeInfo("%s %d) %s <= %s", indent, id, varname, numstr, node->getChild(0));
            printTree(node->getChild(0), level + 1);
            printNodeInfo("%s %d) %s >  %s", indent, id, varname, numstr, node->getChild(1));
            printTree(node->getChild(1), level + 1);
        }
    }

}

void Tree::print () {
    printTree(root_, 0);
    Rprintf("\n");
}

void Tree::save (vector<vector<double> >& res)
/*
 * Store the tree.
 *
 * A node is saved as vector<double>,
 * so the tree is saved as vector<vector<double>>, in breadth-first order and from left to right.
 */
{
    tree_ = vector<vector<double> >(nnodes_);
    doSthOnNodes(root_, &Tree::saveOneNode);
    res.swap(tree_);

}

