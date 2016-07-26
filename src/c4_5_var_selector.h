#ifndef C4_5_VAR_SELECTOR_H_
#define C4_5_VAR_SELECTOR_H_

#include "IGR.h"
#include "var_selector.h"

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
#include <boost/bind.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#else
#include <random>
#endif
#endif

class C4p5Selector: public VarSelector {
private:
    const static int MIN_NODE_SIZE_;  // threshold for minimum child node size

    unsigned seed_;
    double   info_;  // entropy of this node

    map<int, double> info_gain_map_;    // information gain for each variable
    map<int, double> split_info_map_;   // splitinfo for each variable
    map<int, double> split_value_map_;  // <variable> : <optimal split value>
    map<int, map<int, vector<int> > > cand_splits_map_;  // <variable> : "<value> : <observations with that value>"

    void   setResult (int vindex, VarSelectRes& result, double gain_ratio=NA_REAL);
    void   calcInfos (const vector<int>& var_vec, volatile bool* pInterrupt);
    double averageInfoGain ();

public:

    C4p5Selector (Dataset*, TargetData*, MetaData*, const vector<int>&, const vector<int>&, unsigned);

    vector<int> getRandomVars (vector<int> var_vec, int nselect);
    template<class T> void handleContVar (int var_idx);
    void handleContVar (int var_idx);
    void handleDiscVar (int var_idx);
    void doSelection (int nvars, VarSelectRes& res, volatile bool* pInterrupt);     // C4.5
    void doIGRSelection (int nvars, VarSelectRes& res, volatile bool* pInterrupt);  // IGR weight method

    double sumNlogn (const vector<int>& dstr, int nobs) {
        double sum = 0;
        for (vector<int>::const_iterator iter = dstr.begin(); iter != dstr.end(); iter++)
            if (*iter != 0) sum += train_set_->nlogn(*iter);

        return train_set_->nlogn(nobs) - sum;
    }

    double calcEntropy (const vector<int>& obs_vec) {
        int n = obs_vec.size();
        return sumNlogn(targ_data_->getLabelFreqCount(obs_vec), n) / n;
    }

    double calcBisectSubinfo (const vector<int>& ldstr, int lnobs, const vector<int>& rdstr, int rnobs) {
        return (sumNlogn(ldstr, lnobs) + sumNlogn(rdstr, rnobs))/(lnobs+rnobs);
    }

    template<class T>
    struct VarValueComparor {
        //TODO: Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.
        T* var_array_;

        VarValueComparor (Dataset* data, int var_idx)
            : var_array_(data->getVar<T>(var_idx)) {
        }

        bool operator() (int a, int b) {
            return var_array_[a] < var_array_[b] ? true : false;
        }
    };

};

#endif

