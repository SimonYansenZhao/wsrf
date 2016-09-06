#ifndef SRC_SAMPLING_H_
#define SRC_SAMPLING_H_

#include "utility.h"

class Sampling {

private:
    unsigned seed_;
    vector<double> weights_;
    vector<int>    wst_;

public:
    Sampling(unsigned seed);
    vector<int> nonReplaceRandomSample(vector<int> var_vec, int nselect);
    vector<int> nonReplaceWeightedSample(const vector<double>& originalweights, int nselect, volatile bool* pInterrupt, bool needsqrt=true);
    vector<int> nonReplaceWeightedSample(const vector<int>& var_vec, const vector<double>& originalweights, int nselect, volatile bool* pInterrupt, bool needsqrt=true);
};

#endif /* SRC_SAMPLING_H_ */
