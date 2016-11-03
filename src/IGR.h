#ifndef IGR_H_
#define IGR_H_

#include "utility.h"
#include "sampling.h"

using namespace std;

class IGR {
private:
    int nvars_;  //subspace size
    unsigned seed_;

    vector<double> weights_;
    vector<int>    wst_;

    volatile bool* pInterrupt_;
    bool isParallel_;

    const vector<double>& gain_ratio_vec_;

public:
    IGR(const vector<double>& gain_ratio, int nvars, unsigned seed, volatile bool* pInterrupt, bool isParallel);

    int  getSelectedIdx();
};

#endif /* IGR_H_ */
