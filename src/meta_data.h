#ifndef METADATA_H_
#define METADATA_H_

#include "utility.h"

using namespace std;

class MetaData {
    /*
     * Variable meta data interpreter.
     *
     * Provides interface for querying meta data.
     */
private:
    typedef vector<int>               type_vec;
    typedef vector<int>               idx_vec;
    typedef vector<string>            name_vec;
    typedef map<string, int>          name_value_map;
    typedef map<int, name_value_map > var_value_map;
    typedef map<int, name_vec >       val_name_map;

    int            nvars_;          // The total number of feature variables, exclusive of target variable.

    int            targ_var_idx_;   // The index of target variable.
    string         targ_var_name_;  // The name of the target variable.
    int            nlabels_;        // The number of labels of target variable.

    name_vec       var_names_;      // The names of all feature variables, exclusive of target variable.
    type_vec       var_types_;      // The types of all feature variables, exclusive of target variable.
    var_value_map  var_values_;     // The name-value map of all discrete variables, including target variable.  Key: index of discrete variable.
    val_name_map   val_names_;      // The value names of all discrete variables, including target variable.  Key: index of discrete variable.

    idx_vec        feature_vars_;   // The indexes of all feature variables, exclusive of target variable.

public:
    MetaData (Rcpp::DataFrame data, string targ_name);
    MetaData (Rcpp::List md);

    Rcpp::List save () const;

    int nvars () const {
        // The total number of feature variables, exclusive of target variable.
        return nvars_;
    }

    int targVarIdx () const {
        // The index of target variable.
        return targ_var_idx_;
    }

    string targVarName () const {
        // The name of target variable.
        return targ_var_name_;
    }

    int getNumValues (int index) const {
        // The number of possible values taken by the discrete variable <index>.
        val_name_map::const_iterator iter = val_names_.find(index);
        return iter->second.size();
    }

    int nlabels() const {
        // The number of class labels.
        return nlabels_;
    }

    int getVarType (int index) const {
        // The type of a certain feature variable: discrete or continuous.
        return var_types_[index];
    }

    string getValueName (int variable_index, int value_index) const {
        // The value name of <value_index> for the discrete variable <variable_index>.
        val_name_map::const_iterator iter = val_names_.find(variable_index);
        return iter->second[value_index];
    }

    vector<string> getValueNames (int index) const {
        // The level names of the discrete variable <index>.
        val_name_map::const_iterator iter = val_names_.find(index);
        return iter->second;
    }

    vector<string> getLabelNames () const {
        // The names of class labels.
        return getValueNames(targ_var_idx_);
    }

    string getLabelName (int index) const {
        // The name of class label <index>.
        return getValueName(targ_var_idx_, index);
    }

    int getValue (int index, string name) const {
        // The integer value of the discrete variable <index> at level <name>.
        var_value_map::const_iterator iter = var_values_.find(index);
        name_value_map::const_iterator iter2 = iter->second.find(name);
        return iter2->second;
    }

    const string getVarName (int index) const {
        // The name of the feature variable <index>.
        return var_names_[index];
    }

    const vector<string>& getVarNames () {
        // The names of all feature variables.
        return var_names_;
    }

    const vector<int>& getFeatureVars () const {
        // Return feature variable index list.
        return feature_vars_;
    }

};
#endif
