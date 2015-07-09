#ifndef RFORESTS_H_
#define RFORESTS_H_

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/foreach.hpp>
#else
#include <thread>
#include <mutex>
#include <random>
#include <future>
#include <chrono>
#endif
#endif

#include "tree.h"

using namespace std;

class RForest {
private:
    Dataset*    train_set_;   // Training set
    TargetData* targ_data_;
    MetaData*   meta_data_;

    vector<vector<int> > bagging_set_;  // Bagging set for each tree
    vector<vector<int> > oob_set_vec_;
    vector<Tree*>        tree_vec_;

    int  ntrees_;
    unsigned* tree_seeds_;  // Seed for each tree
    int  nlabels_;
    bool importance_;
    int  mtry_;
    bool weights_;

    double rf_oob_error_rate_;
    double rf_strength_;
    double rf_correlation_;
    double c_s2_;
    double emr2_;


    vector<vector<int> > oob_predict_label_freq_matrix_;  // Matrix of size nobs*nlabels: Predicted label frequency count for each observation in OOB.
    vector<int>          oob_predict_label_vec_;          // Vector of size nobs: Predicted label for each observation in OOB, max predicted label, -1 by default, that is not in OOB.
    vector<int>          oob_count_vec_;                  // Vector of size nobs: Number of times the observation being in OOB.
    vector<double>       oob_confusion_matrix_;           // Vector of size (nlabels+1)*nlabels: Confusion matrix for OOB, row - predicted label and class error, column - actual label.
    vector<int>          max_j_;

    vector<double> raw_perm_VIs_;    // Vector of size (nlabels+1)*nvars: Raw vairable importance on each class label, before scaled, plus one row for VI over all class labels.
    vector<double> sigma_perm_VIs_;  // Vector of size (nlabels+1)*nvars: Standard deviation of variable impaortance on each class label, , plus one row for VI SD over all class labels.
    vector<double> IGR_VIs_;         // Vector of size nvars: The information gain ratio decreases for each variable.

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
    boost::mutex mut;
#else
    mutex mut;
#endif
#endif


    typedef void (RForest::*predictor)(Dataset* data, int index, double* out_iter);

    void collectBasicStatistics ();
    void calcOOBConfusionErrorRateAndStrength ();
    void calcRFCorrelationAndCS2 ();
    void assessPermVariableImportance ();

public:

    RForest (Rcpp::List& model_list, MetaData* meta_data, TargetData* targdata);
    RForest (Dataset* training_set, TargetData* targdata, MetaData* meta_data, int tree_num, int nvars, bool weights, bool importance, SEXP seeds);
    ~RForest ();

    void predictLabelFreqCount (Dataset* data, int index, double* out_iter) {
        /*
         * Predicted label frequency count by all trees for data[index].
         */
        for (vector<Tree*>::iterator iter = tree_vec_.begin(); iter != tree_vec_.end(); ++iter)
            out_iter[(*iter)->predictLabel(data, index)]++;

    }

    void predictProbVec (Dataset* data, int index, double* out_iter) {
        /*
         * Predicted label frequency by all trees for data[index].
         */
        for (vector<Tree*>::iterator iter = tree_vec_.begin(); iter != tree_vec_.end(); ++iter)
            out_iter[(*iter)->predictLabel(data, index)]++;

        for (int i = 0; i < nlabels_; i++)
            out_iter[i] /= ntrees_;

    }

    void predictAprobVec (Dataset* data, int index, double* out_iter) {
        for (vector<Tree*>::iterator iter = tree_vec_.begin(); iter != tree_vec_.end(); ++iter) {
            vector<double> classDistributions = (*iter)->predictLeafNode(data, index)->getLabelDstr();
            for (int i = 0; i < nlabels_; i++)
                out_iter[i] += classDistributions[i];
        }

        for (int i = 0; i < nlabels_; i++)
            out_iter[i] /= ntrees_;

    }

    void predictWAprobVec (Dataset* data, int index, double* out_iter) {
        double sumAccuracy = 0;
        for (vector<Tree*>::iterator iter = tree_vec_.begin(); iter != tree_vec_.end(); ++iter) {
            vector<double> classDistributions = (*iter)->predictLeafNode(data, index)->getLabelDstr();
            double accuracy = 1 - (*iter)->getTreeOOBErrorRate();
            sumAccuracy += accuracy;
            for (int i = 0; i < nlabels_; i++)
                out_iter[i] += classDistributions[i] * accuracy;
        }

        for (int i = 0; i < nlabels_; i++)
            out_iter[i] /= sumAccuracy;

    }

    Rcpp::NumericMatrix predictMatrix (Dataset* data, predictor pred);
    Rcpp::IntegerVector predictClassVec (Dataset* data);

    void saveModel (Rcpp::List& wsrf_R);
    void saveMeasures (Rcpp::List& wsrf_R);

    void calcEvalMeasures () {
        int nobs    = targ_data_->nobs();

        max_j_                         = vector<int>(nobs, -1);
        oob_predict_label_vec_         = vector<int>(nobs, NA_INTEGER);
        oob_count_vec_                 = vector<int>(nobs, 0);
        oob_predict_label_freq_matrix_ = vector<vector<int> >(nobs, vector<int>(nlabels_, 0));
        oob_confusion_matrix_          = vector<double>((nlabels_+1)*nlabels_, 0);
        IGR_VIs_                       = vector<double>(meta_data_->nvars(), 0),

        collectBasicStatistics();
        calcOOBConfusionErrorRateAndStrength();
        calcRFCorrelationAndCS2();
        if (importance_) assessPermVariableImportance();
    }

    void buildOneTree (int ind, volatile bool* interrupt);
    void buidForestSeq (volatile bool* pinterrupt);
#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
    // parallel: 0 or 1 (sequential);  < 0 (cores-2 threads); > 1 (the exact num of threads)
    void buildForestAsync (int parallel, volatile bool* pInterrupt);
    void buildOneTreeAsync (int* index, volatile bool* pInterrupt);
#endif

};

#endif
