#ifndef IGR_H_
#define IGR_H_

#include<random>

#include "utility.h"

using namespace std;

class IGR {
private:
    int nvars_;  //subspace size
    unsigned seed_;

    vector<double> weights_;
    vector<int>    wst_;

    const vector<double>& gain_ratio_vec_;


    int weightedSampling(int random_integer);
    inline vector<int> getRandomWeightedVars();
public:
    IGR(const vector<double>& gain_ratio, int nvars, unsigned seed);

    void normalizeWeight(volatile bool* pInterrupt);
    int  getSelectedIdx();
};

#endif /* IGR_H_ */
