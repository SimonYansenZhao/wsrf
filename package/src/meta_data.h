#ifndef METADATA_H_
#define METADATA_H_

#include "utility.h"

using namespace std;

/*
 * Variable meta data interpreter.
 *
 * Provides interface for querying meta data.
 */
class MetaData {
private:
    typedef vector<int>               type_vec;
    typedef vector<int>               idx_vec;
    typedef vector<string>            name_vec;
    typedef map<string, int>          name_value_map;
    typedef map<int, name_value_map > var_value_map;
    typedef map<int, name_vec >       val_name_map;

    int            nvars_;          // The total number of feature variables.
    int            targ_var_idx_;   // The index of target variable.
    string         targ_var_name_;  // The name of the target variable.
    int            nlabels_;        // The number of labels of target variable.
    name_vec       var_names_;      // The names of all feature variables.
    type_vec       var_types_;      // The types of all feature variables.
    var_value_map  var_values_;     // The name-value map of all discrete variables, including target variable.
    val_name_map   val_names_;      // The value names of all discrete variables, including target variable.

    idx_vec        feature_vars_;

public:
    MetaData (Rcpp::DataFrame data, string targ_name);
    MetaData (Rcpp::List md);

    Rcpp::List save () const;

    // The total number of feature variables, excluding target variable.
    int nvars () const {
        return nvars_;
    } //***

    // The index of target variable.
    int targVarIdx () const {
        return targ_var_idx_;
    } //***

    // The name of target variable.
    string targVarName () const {
        return targ_var_name_;
    }

    // The number of possible values taken by a certain discrete variable.
    int getNumValues (int index) const {
        val_name_map::const_iterator iter = val_names_.find(index);
        return iter->second.size();
    } //***

    int nlabels() const {
        // The number of class labels.

        return nlabels_;
    } //***

    // The type of a certain variable: discrete or continuous.
    int getVarType (int index) const {
        return var_types_[index];
    } //***

    // The string value for a integer value of a variable.
    string getValueName (int variable_index, int value_index) const {
        val_name_map::const_iterator iter = val_names_.find(variable_index);
        return iter->second[value_index];
    }

    vector<string> getValueNames (int index) const {
        val_name_map::const_iterator iter = val_names_.find(index);
        return iter->second;
    }

    vector<string> getLabelNames () const {
        return getValueNames(targ_var_idx_);
    } //***

    string getLabelName (int index) const {
        return getValueName(targ_var_idx_, index);
    }

    // The integer value for a string value of a variable.
    int getValue (int index, string name) const {
        var_value_map::const_iterator iter = var_values_.find(index);
        name_value_map::const_iterator iter2 = iter->second.find(name);
        return iter2->second;
    }

    // The name of a variable.
    const string getVarName (int index) const {
        return var_names_[index];
    }

    const vector<string>& getVarNames () {
        return var_names_;
    }

    const vector<int>& getFeatureVars () const {
        /*
         * Return feature variable index list.
         */
        return feature_vars_;
    }

};
#endif
