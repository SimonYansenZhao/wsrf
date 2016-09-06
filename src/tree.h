#ifndef TREE_H_
#define TREE_H_

#include <iostream>
#include <queue>

#include "c4_5_var_selector.h"
#include "dataset.h"
#include "node.h"

using namespace std;

class Tree {
private:

    unsigned    seed_;                  // Random seed for this tree.
    Node*       root_;                  // Root node of the tree.
    Dataset*    train_set_;             // Training set the tree built from.
    TargetData* targ_data_;
    MetaData*   meta_data_;             // Meta data.
    int         nnodes_;                // Total number of nodes in the tree.
    int         node_id_;               // For printing tree.
    double      tree_oob_error_rate_;   // Out-of-bag error rate.
    int         min_node_size_;         // Minimum node size.
    int         mtry_;                  // Number of variables selected for node splitting.
    bool        isweight_;              // Whether weighting.
    bool        isimportance_;          // Whether calculate variable importance.

    vector<double> label_oob_error_rate_;  // Vector of size nlabels: The OOB error rate for each class label.

    vector<vector<double> > tree_;     // Serialized tree.

    vector<int>* pbagging_vec_;  // Bagging set
    vector<int>* poob_vec_;      // Out-of-bag set: The size of it may be one third of the number of observations.

    vector<int> oob_predict_label_set_;  // The predicted labels for Out-of-bag set: The same size of *poob_vec_.

    int            perm_var_idx_;      // Should variable importance be assessed (-1), or otherwise, the index of current permuted variable.
    vector<bool>   perm_is_var_used_;  // Vector of size nvars: Indicate whether the variable is used for node splitting in this tree.
    vector<double> perm_var_data_;     // Vector of size nobs: Permuted data of variable perm_var_idx_.
    vector<double> tree_IGR_VIs_;      // Vector of size nvars: The information gain ratio decreases for each variable.
    vector<double> tree_perm_VIs_;     // Matrix of (nlabels+1)*nvars: The percent increases of OOB error rate on each class label in the permuted OOB data, plus one over all class labels.

    vector<int> removeOneVar (const vector<int>& var_vec, int index)
    /*
     * Remove a <index> from <var_vec>.
     */
    {
        int n = var_vec.size();
        vector<int> res(n - 1);
        for (int i = 0, j = 0; i < n; i++)
            if (var_vec[i] != index) res[j++] = var_vec[i];

        return res;
    }

    template<class T>
    static double getDataValue (Tree* tree, Dataset* data_set, int vindex, int oindex) {
        if (vindex != tree->perm_var_idx_) {
            return data_set->getValue<T>(vindex, oindex);
        } else {
            return (tree->perm_var_data_)[oindex];
        }
    }

    Node* predictNode (Dataset* data_set, int oindex, Node* node)
    /*
     * Return leaf node to which the observation belongs.
     * index : is the index of the observation in the training set
     */
    {
        while (node->type() != LEAFNODE) {
            int vindex = node->getVarIdx();
            double value;

            switch (meta_data_->getVarType(vindex)) {
            case DISCRETE:
                node = node->getChild(getDataValue<int>(this, data_set, vindex, oindex) - 1);
                continue;
                break;
            case INTSXP:
                value = getDataValue<int>(this, data_set, vindex, oindex);
                break;
            case REALSXP:
                value = getDataValue<double>(this, data_set, vindex, oindex);
                break;
            default:
                throw std::range_error("Unexpected variable type for " + meta_data_->getVarName(vindex));
                break;
            }

            if (value <= node->getSplitValue()) node = node->getChild(0);
            else node = node->getChild(1);
        }

        return node;
    }

    void printNodeInfo (const char* format, string& indent, int id, string& varname, const char* value, Node* child) {
        Rprintf(format, indent.c_str(), id, varname.c_str(), value);

        if (child->type() == LEAFNODE) {
            string labelname = meta_data_->getLabelName(child->label());
            string dstr = child->getLabelDstrStr();
            Rprintf("   [%s] (%s) *", labelname.c_str(), dstr.c_str());
        }

        Rprintf("\n");
    }


    typedef void (Tree::*Dosth)(Node* node, int nth_iter);

    void doSthOnNodes (Node* root, Dosth dosth)
    /*
     * Traverse the tree rooted at <root>, and operate <dosth> on the node.
     */
    {
        queue<Node*> untraversed_nodes;

        untraversed_nodes.push(root);

        for (int i = 0; !untraversed_nodes.empty(); i++) {
            Node* node = untraversed_nodes.front();
            int   n    = node->nchild();  // Leaf node has no child.
            for (int j = 0; j < n; j++)
                untraversed_nodes.push(node->getChild(j));

            (this->*dosth)(node, i);
            untraversed_nodes.pop();
        }

    }

    void saveOneNode (Node* node, int nth_iter) {
        node->save(tree_[nth_iter], meta_data_);
    }

    void markOneVarUsed (Node* node, int nth_iter)
    /*
     * Check whether the node is internal node and mark the variable as used for node splitting.
     */
    {
        if (node->type() != LEAFNODE)
            perm_is_var_used_[node->getVarIdx()] = true;
    }

    void deleteTheNode (Node* node, int nth_iter) {
        if (node != NULL)
            delete node;
    }

    void printTree (Node* node, int level);
    void calcOOBMeasures (bool importance);

    template<class T>
    void copyPermData ();

public:

    Tree (const vector<vector<double> >& node_infos, MetaData* meta_data, double tree_oob_error_rate);
    Tree (Dataset*, TargetData*, MetaData*, int, unsigned int, vector<int>*, vector<int>*, int, bool, bool);

    ~Tree () {
        doSthOnNodes(root_, &Tree::deleteTheNode);
    }

    vector<double>& getTreePermVIs () {
        return tree_perm_VIs_;
    }

    vector<double>& getTreeIGRVIs () {
        return tree_IGR_VIs_;
    }

    void setTreeIGRVIs (vector<double>& tree_IGR_VIs) {
        tree_IGR_VIs_.swap(tree_IGR_VIs);
    }

    double getTreeOOBErrorRate () {
        return tree_oob_error_rate_;
    }

    vector<int>& getOOBPredictLabelSet () {
        return oob_predict_label_set_;
    }

    void setOOBPredictLabelSet (vector<int>& oob_predict_label_set) {
        oob_predict_label_set_.swap(oob_predict_label_set);
    }

    Node* genC4p5Tree (
        const vector<int>& training_set_index,
        const vector<int>& attribute_list,
        volatile bool* pInterrupt);

    Node* createLeafNode (const vector<int>& obs_vec, int nobs, bool pure)
    /*
     * Create a leaf node.
     * Because all observations are the same label (pure = true),
     * or there is no better variable to split (pure = false).
     */
    {
        ++nnodes_;
        Node* node = new Node(LEAFNODE, nobs);
        if (pure) {  // All observations have the same label.
            int label = targ_data_->getLabel(obs_vec[0]) - 1;
            node->setLabel(label);

            vector<int> numbers(meta_data_->nlabels(), 0);
            numbers[label] = obs_vec.size();
            node->setLabelFreqCount(numbers);
        } else {  // There is no better variable to split.
            node->setLabelFreqCount(targ_data_->getLabelFreqCount(obs_vec), true);
        }
        return node;
    }

    Node* createInternalNode (int nobs, VarSelectRes& res)
    /*
     * Create a internal node.
     * It represents the variable selected to split the data,
     * with related impurity measures.
     */
    {
        ++nnodes_;
        Node* node = new Node(INTERNALNODE, nobs, res.split_map_.size());
        node->setVarIdx(res.var_idx_);
        node->setInfoGain(res.info_gain_);
        node->setSplitInfo(res.split_info_);
        node->setGainRatio(res.gain_ratio_);
        if (res.gain_ratio_ != NA_REAL)
            tree_IGR_VIs_[res.var_idx_] += res.gain_ratio_;
        return node;
    }

    Node* predictLeafNode (Dataset* data_set, int index) {
        return predictNode(data_set, index, root_);
    }

    int predictLabel (Dataset* data_set, int index) {
        return predictLeafNode(data_set, index)->label();
    }

    void print ();
    void build (volatile bool* pinterrupt);
    void save (vector<vector<double> >& res);

    void permute (int index);
    void resetPerm (bool initial);

    void genBaggingSets ();

};

#endif
