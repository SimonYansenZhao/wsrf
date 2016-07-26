#include "meta_data.h"

MetaData::MetaData (Rcpp::DataFrame data, string targ_name) {
    /*
     * Obtain meta data directly from data set.
     *
     * Used for training.
     *
     * Assume that the target variable is the last variable,
     * and no unused variables are in argument <data>
     */
    nvars_         = data.size() - 1;
    targ_var_name_ = targ_name;

    if (nvars_ == 0) throw std::range_error("Dataset is empty.");

    targ_var_idx_ = nvars_;
    feature_vars_ = idx_vec(nvars_);
    var_names_    = name_vec(nvars_);
    var_types_    = type_vec(nvars_);

    Rcpp::CharacterVector vnames(data.names());
    for (int vindex = 0; vindex < nvars_; vindex++) {
        // Store the names of feature variables.
        var_names_[vindex] = Rcpp::as<string>(vnames[vindex]);
    }

    for (int vindex = 0; vindex < nvars_; vindex++) {
        if (Rf_isFactor(data[vindex])) {
            // Store the levels of factor variables.
            Rcpp::IntegerVector vals(data[vindex]);
            Rcpp::CharacterVector levels(vals.attr("levels"));
            int nlevels = levels.size();

            name_value_map namevals;
            name_vec levnames(nlevels);
            for (int lindex = 0; lindex < nlevels; lindex++) {
                string name = Rcpp::as<string>(levels[lindex]);
                namevals.insert(name_value_map::value_type(name, lindex));  // Here, factor values starts from 0.
                levnames[lindex] = name;
            }
            var_values_[vindex].swap(namevals);
            val_names_[vindex].swap(levnames);
            var_types_[vindex] = DISCRETE;
        } else {
            var_types_[vindex] = TYPEOF(data[vindex]);
        }
    }

    // Store the levels of the target variable.
    Rcpp::IntegerVector vals(data[targ_var_idx_]);
    Rcpp::CharacterVector levels(vals.attr("levels"));
    int nlevels = levels.size();

    name_value_map namevals;
    name_vec levnames(nlevels);
    for (int lindex = 0; lindex < nlevels; lindex++) {
        string name = Rcpp::as<string>(levels[lindex]);
        namevals.insert(name_value_map::value_type(name, lindex));
        levnames[lindex] = name;
    }
    var_values_[targ_var_idx_].swap(namevals);
    val_names_[targ_var_idx_].swap(levnames);

    nlabels_ = val_names_[targ_var_idx_].size();

    // Store the indexes of feature variables.
    for (int i = 0; i < nvars_; ++i)
        feature_vars_[i] = i;
}

MetaData::MetaData (Rcpp::List md) {
    /*
     * Construct meta data from the R list of meta data.
     *
     * Used for prediction.
     *
     * Format:
     * [[0]] ---- Number of variables
     * [[1]] ---- Index of target variable
     * [[2]] ---- Target variable name
     * [[3]] ---- Variable name vector
     * [[4]] ---- Variable type vector
     * [[5]] ---- Discrete variable value list
     */
    nvars_         = Rcpp::as<int>(md[NVARS]);
    targ_var_idx_  = Rcpp::as<int>(md[TARG_IDX]);
    targ_var_name_ = Rcpp::as<string>(md[TARG_NAME]);
    var_types_     = type_vec(Rcpp::as<vector<int> >(md[VAR_TYPES]));

    feature_vars_ = idx_vec(nvars_);
    var_names_    = name_vec(nvars_);

    Rcpp::CharacterVector varnames((SEXP)(md[VAR_NAMES]));
    for (int vindex = 0; vindex < nvars_; vindex++) {
        string name = Rcpp::as<string>(varnames[vindex]);
        var_names_[vindex] = name;
    }

    Rcpp::List valuenames((SEXP)(md[VAL_NAMES]));
    for (int i = 0; i < valuenames.size(); i++) {
        Rcpp::List lst(valuenames[i]);
        int vindex = Rcpp::as<int>(lst[0]);
        Rcpp::CharacterVector levels(lst[1]);
        int nlevels = levels.size();

        name_vec levnames(nlevels);
        name_value_map namevals;
        for (int lindex = 0; lindex < nlevels; lindex++) {
            string name = Rcpp::as<string>(levels[lindex]);
            namevals.insert(name_value_map::value_type(name, lindex));
            levnames[lindex] = name;
        }
        var_values_[vindex].swap(namevals);
        val_names_[vindex].swap(levnames);
    }

    for (int i = 0; i < nvars_; ++i)
        feature_vars_[i] = i;

    nlabels_ = val_names_[targ_var_idx_].size();

}

Rcpp::List MetaData::save () const {
    /*
     * Save meta data into R list.
     *
     * Format:
     * [[0]] ---- Number of variables
     * [[1]] ---- Index of target variable
     * [[2]] ---- Target variable name
     * [[3]] ---- Variable name vector
     * [[4]] ---- Variable type vector
     * [[5]] ---- Discrete variable value list
     */

    Rcpp::List meta_data;

    meta_data[NVARS]     = Rcpp::wrap(nvars_);
    meta_data[TARG_IDX]  = Rcpp::wrap(targ_var_idx_);
    meta_data[TARG_NAME] = Rcpp::wrap(targ_var_name_);
    meta_data[VAR_NAMES] = Rcpp::wrap(var_names_);
    meta_data[VAR_TYPES] = Rcpp::wrap(var_types_);

    Rcpp::List valuenames;
    for (val_name_map::const_iterator iter = val_names_.begin(); iter != val_names_.end(); ++iter) {
        Rcpp::List vn;
        vn.push_back(iter->first);
        vn.push_back(iter->second);
        valuenames.push_back(vn);
    }
    meta_data[VAL_NAMES] = Rcpp::wrap(valuenames);

    return meta_data;
}
