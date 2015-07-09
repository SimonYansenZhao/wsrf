#ifndef DATASET_H_
#define DATASET_H_

#include "utility.h"
#include "meta_data.h"

using namespace std;

class TargetData {
private:
    int  nlabels_;
    int  nobs_;
    int* targ_array_;

    Rcpp::IntegerVector data_;

public:
    TargetData (Rcpp::DataFrame ds, MetaData* meta_data) {
        nlabels_ = meta_data->nlabels();
        nobs_    = ds.nrows();

        data_       = Rcpp::as<Rcpp::IntegerVector>(ds[meta_data->targVarIdx()]);
        targ_array_ = INTEGER(data_);
    }

    TargetData (Rcpp::List targdata) {
        nlabels_ = Rcpp::as<int>(targdata[NLABELS]);

        data_ = Rcpp::as<Rcpp::IntegerVector>(targdata[TRAIN_TARGET_LABELS]);
        targ_array_ = INTEGER(data_);
        nobs_       = data_.size();
    }

    int nobs () {
        return nobs_;
    }

    int nlabels () {
        return nlabels_;
    }

    int getLabel (int index) {
        // The class label of the observation.
        return targ_array_[index];
    }

    bool haveSameLabel (const vector<int>& obs_vec) {
        // Whether all observations have the same class label.

        int nobs = obs_vec.size();
        if (nobs == 0) {
            throw std::range_error("Empty node.");
        } else if (nobs == 1) {
            return true;
        } else {
            int label = targ_array_[obs_vec[0]];
            for (int i = 1; i < nobs; i++)
                if (label != targ_array_[obs_vec[i]]) return false;
            return true;

        }
    }

    vector<int> getLabelFreqCount (const vector<int>& obs_vec) {
        // Class label frequency count.
        // That is, how many observations have a specific class label.

        int nobs = obs_vec.size();
        vector<int> numbers (nlabels_, 0);

        for (int i = 0; i < nobs; i++)
            numbers[targ_array_[obs_vec[i]] - 1]++;

        return numbers;
    }

    Rcpp::List save () {
        Rcpp::List res;

        res[NLABELS]             = Rcpp::wrap(nlabels_);
        res[TRAIN_TARGET_LABELS] = Rcpp::clone(data_);

        return res;
    }

};

class Dataset {
private:
    vector<void*>    data_ptr_vec_;
    MetaData*        meta_data_;     // Meta data.
    int              nobs_;          // The total number of observations in the training set.
    bool             training_;
    vector<double>   nlogn_vec_;
    vector<Rcpp::IntegerVector> preserve_int;
    vector<Rcpp::NumericVector> preserve_num;

    void init (int pindex, SEXP dataptr) {
        //TODO: Ugly hack! Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.
        switch (meta_data_->getVarType(pindex)) {
        case DISCRETE:
            if (!training_) {
                Rcpp::IntegerVector rcppdata(dataptr);
                preserve_int.push_back(rcppdata);  // Preserve data in case that if the type of the variable is different from training data, <rcppdata> would be destroyed out of the code block.
                int nlevels_actual = Rcpp::CharacterVector(rcppdata.attr("levels")).size();
                int nlevels_known = meta_data_->getNumValues(pindex);

                if (nlevels_known < nlevels_actual) {  // There are more values than in training data.

                    throw std::range_error("New values that do not exist in training data found in variable "
                        + meta_data_->getVarName(pindex) + "!");

                } else if (nlevels_known == nlevels_actual) {

                    vector<string> actual_levels = Rcpp::as<vector<string> >(rcppdata.attr("levels"));
                    vector<string> known_levels = meta_data_->getValueNames(pindex);

                    if (equal(actual_levels.begin(), actual_levels.end(), known_levels.begin())) {
                        data_ptr_vec_[pindex] = INTEGER(rcppdata);
                    } else throw std::range_error("Mismatch values between training data and predicting data found in variable "
                        + meta_data_->getVarName(pindex) + "!");

                } else {  // There are less values than in training data.  We need to match the values with training data.

                    vector<string> sorted_actual_levels = Rcpp::as<vector<string> >(rcppdata.attr("levels"));
                    vector<string> sorted_known_levels = meta_data_->getValueNames(pindex);

                    sort(sorted_actual_levels.begin(), sorted_actual_levels.end());  // Sorting results between R and C++ are different.
                    sort(sorted_known_levels.begin(), sorted_known_levels.end());

                    if (includes(sorted_known_levels.begin(), sorted_known_levels.end(), sorted_actual_levels.begin(), sorted_actual_levels.end())) {

                        vector<string> actual_levels = Rcpp::as<vector<string> >(rcppdata.attr("levels"));  // Use unsorted levels.
                        vector<int> match_vec(actual_levels.size() + 1);
                        for (int i = 0; i < nlevels_actual; i++)
                            match_vec[i + 1] = meta_data_->getValue(pindex, actual_levels[i]) + 1;
                        int* pdata = INTEGER(rcppdata);
                        for (int i = 0; i < nobs_; i++)
                            pdata[i] = match_vec[pdata[i]];
                        data_ptr_vec_[pindex] = pdata;

                    } else throw std::range_error("New values that do not exist in training data found in variable "
                        + meta_data_->getVarName(pindex) + "!");

                }
                break;
            }
            /* no break */
        case INTSXP:
            if (!training_) {
                preserve_int.push_back(Rcpp::IntegerVector(dataptr));  // Preserve data in case that the type of the variable is different from training data.
                data_ptr_vec_[pindex] = INTEGER(preserve_int.back());
            } else {
                data_ptr_vec_[pindex] = INTEGER(Rcpp::IntegerVector(dataptr));
            }
            break;
        case REALSXP:
            if (!training_) {
                preserve_num.push_back(Rcpp::NumericVector(dataptr));  // Preserve data in case that the type of the variable is different from training data.
                data_ptr_vec_[pindex] = REAL(preserve_num.back());
            } else {
                data_ptr_vec_[pindex] = REAL(Rcpp::NumericVector(dataptr));
            }
            break;
        default:
            throw std::range_error("Unexpected variable type for " + meta_data_->getVarName(pindex) + ": Only integer, numeric and factor supported now.");
            break;
        }
    }

public:

    Dataset (Rcpp::DataFrame ds, MetaData* meta_data, bool training);

    double nlogn (int n) {
        return nlogn_vec_[n];
    }

    int nobs () const {
        return nobs_;
    }

    template<class T>
    T* getVar (int index) {
        //TODO: Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.
        return (T*)(data_ptr_vec_[index]);
    }

    template<class T>
    T getValue (int vindex, int oindex) {
        //TODO: Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.
        return getVar<T>(vindex)[oindex];
    }

    map<int, vector<int> > splitDiscVar (const vector<int>&, int);
    map<int, vector<int> > splitPosition (vector<int>&, int);

};
#endif
