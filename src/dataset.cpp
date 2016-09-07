#include "dataset.h"

Dataset::Dataset (SEXP xSEXP, MetaData* meta_data, bool training) {
    Rcpp::DataFrame ds(xSEXP);
    training_     = training;
    nobs_         = ds.nrows();
    data_ptr_vec_ = vector<void*>(ds.size());
    meta_data_    = meta_data;

    if (nobs_ == 0) throw std::range_error("No observations in the dataset.");

    int nvars = meta_data_->nvars();
    if (training_) {
        /*
         * For training data set.
         */

        for (int i = 0; i < nvars; i++)
            this->init(i, (SEXPREC*)ds[i]);

        int n = 1;
        nlogn_vec_ = vector<double>(ds.nrows()+1);
        for (vector<double>::iterator iter = ++(nlogn_vec_.begin()); iter != nlogn_vec_.end(); iter++, n++) {
            (*iter) = n * log((double)n) / LN_2;
        }

    } else {
        /*
         * For prediction.
         */
        if (nvars > ds.size()) throw std::range_error("The number of variables is less than expected.");

        Rcpp::CharacterVector vnames(ds.names());
        for (int i = 0; i < nvars; i++) {
            if (Rcpp::as<string>((SEXPREC*)vnames[i]) == meta_data_->getVarName(i)) {
                this->init(i, (SEXPREC*)ds[i]);
            } else {
                this->init(i, ds[meta_data_->getVarName(i)]);
            }
        }
    }

}

map<int, vector<int> > Dataset::splitDiscVar (const vector<int>& obs_vec, int vindex)
/*
 * Return a mapping table which elements are
 * <value>:<index list of observations with that same value> pairs
 * where values are possible ones of that variable.
 */
{

    int nvals = meta_data_->getNumValues(vindex);
    int nobs = obs_vec.size();
    int * var_array = getVar<int>(vindex);

    map<int, vector<int> > result;

    for (int i = 0; i < nvals; i++)
        result.insert(map<int, vector<int> >::value_type(i, vector<int>()));

    for (int i = 0; i < nobs; ++i) {
        int val = var_array[obs_vec[i]] - 1;
        result[val].push_back(obs_vec[i]);
    }
    return result;

}

map<int, vector<int> > Dataset::splitPosition (vector<int>& obs_vec, int pos)
/*
 * Separate <obs_vec> into two parts,
 * and the split point is at index <pos>.
 */
{

    int nobs = obs_vec.size();
    map<int, vector<int> > result;
    if (pos < 0 || pos >= nobs) {
        throw std::range_error("wrong in TrainingSet::SplitByPositon");
    } else {
        vector<int> vec0(pos + 1);
        for (int i = 0; i <= pos; ++i)
            vec0[i] = obs_vec[i];
        result[0].swap(vec0);

        vector<int> vec1(nobs - pos - 1);
        for (int i = pos + 1, j = 0; i < nobs; ++i, ++j)
            vec1[j] = obs_vec[i];
        result[1].swap(vec1);
    }
    return result;
}
