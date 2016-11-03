#ifndef NODE_H_
#define NODE_H_

#include <algorithm>
#include <iostream>
#include <sstream>

#include "meta_data.h"

class Node {
private:

    NodeType type_;  // Node type.
    int      nobs_;  // The number of observations represented by the node.

    /*
     * following attributes are for internal node
     */

    int    var_idx_;      // The index of the variable represented by the node.
    double split_value_;  // For continuous variables.
    double info_gain_;    // Information Gain = Info - \sum(\frac{nobs_{child}}{nobs_}Info_{child})
    double split_info_;   // Split Info = - \sum\frac{nobs_{child}}{nobs_} \times \log_2\frac{nobs_{child}}{nobs_}
    double gain_ratio_;   // Information Gain Ratio = Information Gain / Split Info

    vector<Node*> child_nodes_;  // Children nodes of this node.

    /*
     * following attributes are for leaf node
     */

    int label_;   // class label of the leaf node

    vector<int>    label_freq_count_;    // The numbers of observations belong to each label.
    vector<double> label_distribution_;  // = label_freq_count_ / nobs_

public:

    Node (NodeType type, int nobs, int nchild = 0) {
        type_ = type;
        nobs_ = nobs;

        // If it is internal node, initialize children node vector.
        if (nchild != 0) child_nodes_ = vector<Node*>(nchild);
    }

    Node (const vector<double>& node_info, MetaData* meta_data) {
        /*
         * Reconstruct Node
         * see "Node::save ()" for details
         */

        vector<double>::const_iterator iter = node_info.begin();

        type_ = (NodeType) (*iter++);
        nobs_ = *iter++;

        if (type_ == LEAFNODE) {

            label_ = *iter++;

            while (iter != node_info.end())
                label_freq_count_.push_back(*iter++);

        } else {

            child_nodes_ = vector<Node*>((int) (*iter++));
            var_idx_     = *iter++;
            info_gain_   = *iter++;
            split_info_  = *iter++;
            gain_ratio_  = *iter++;

            if (meta_data->getVarType(var_idx_) != DISCRETE) split_value_ = *iter++;
        }
    }

    NodeType type () {
        // Internal node or leaf node.
        return type_;
    }

    void setLabel (int label) {
        // Set the class label represented by this leaf node.
        label_ = label;
    }

    int label () {
        // The class label represented by this leaf node.
        return label_;
    }

    void setVarIdx (int attribute) {
        var_idx_ = attribute;
    }

    int getVarIdx () {
        return var_idx_;
    }

    void setInfoGain (double info_gain) {
        info_gain_ = info_gain;
    }

    void setSplitInfo (double split_info) {
        split_info_ = split_info;
    }

    void setSplitValue (double split_value) {
        split_value_ = split_value;
    }

    double getSplitValue () {
        return split_value_;
    }

    void setGainRatio (double gain_ratio) {
        gain_ratio_ = gain_ratio;
    }

    double getGainRatio () {
        return gain_ratio_;
    }

    int nchild () {
        return child_nodes_.size();
    }

    void setLabelFreqCount (vector<int> label_nums, bool set_label = false) {
        // If set_label = true, calculate and set the major label from label frequency count.
        // Otherwise, just set the label frequency count.

        label_freq_count_.swap(label_nums);

        if (set_label) label_ = distance(label_freq_count_.begin(), max_element(label_freq_count_.begin(), label_freq_count_.end()));
    }

    void setChild (int index, Node* node) {
        child_nodes_[index] = node;
    }

    Node* getChild (int index) {
        return child_nodes_[index];
    }

    vector<double> getLabelDstr () {

        if (type_ != LEAFNODE) throw range_error(INER_ERR_NON_LEAF_NODE_MSG);

        if (label_distribution_.size() <= 0) {
            int n = label_freq_count_.size();
            label_distribution_ = vector<double>(n);

            int nobs = nobs_;
            if (nobs == 0)
                for (int i = 0; i < n; i++)
                    nobs += label_freq_count_[i];

            for (int i = 0; i < n; i++)
                label_distribution_[i] = label_freq_count_[i] / (double) nobs;

        }

        return label_distribution_;
    }

    string getLabelDstrStr ()
    /*
     * For printing the class distribution in the leaf node.
     */
    {
        vector<double> dstr = getLabelDstr();
        stringstream res;
        res.precision(2);

        int n = dstr.size() - 1;
        for (int i = 0; i < n; i++)
            res << dstr[i] << " ";

        res << dstr[n];

        return res.str();
    }

    void save (vector<double>& res, MetaData* meta_data)
    /*
     *     0. type
     *     1. number of observations
     *
     * Leaf node structure:
     *     2. class label
     *     3. label frequency count
     *
     * Internal node structure:
     *     2. number of child nodes
     *     3. variable index
     *     4. information gain
     *     5. split info
     *     6. information gain ratio
     *     7. split value (optional,depends on 3.)
     */
    {
        vector<double> node_info;
        node_info.push_back((int) (type_));
        node_info.push_back(nobs_);

        if (type_ == LEAFNODE) {

            node_info.push_back(label_);

            int n = label_freq_count_.size();
            for (int i = 0; i < n; i++)
                node_info.push_back(label_freq_count_[i]);

        } else {

            node_info.push_back(child_nodes_.size());
            node_info.push_back(var_idx_);
            node_info.push_back(info_gain_);
            node_info.push_back(split_info_);
            node_info.push_back(gain_ratio_);
            if (meta_data->getVarType(var_idx_) != DISCRETE) node_info.push_back(split_value_);

        }

        res.swap(node_info);

    }

};

#endif
