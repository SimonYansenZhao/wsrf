#ifndef VAR_SELECTOR_H_
#define VAR_SELECTOR_H_

#include "dataset.h"

using namespace std;

class VarSelector {
protected:
    Dataset*    train_set_;
    TargetData* targ_data_;
    MetaData*   meta_data_;
    int         nobs_;  // size of obs_vec_

    const vector<int>& obs_vec_;
    const vector<int>& var_vec_;

public:

    VarSelector (Dataset* train_set, TargetData* targdata, MetaData* meta_data, const vector<int>& obs_vec, const vector<int>& var_vect)
        : obs_vec_(obs_vec),
          var_vec_(var_vect) {
        nobs_ = obs_vec.size();
        train_set_ = train_set;
        targ_data_ = targdata;
        meta_data_ = meta_data;
    }

};

#endif

