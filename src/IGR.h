#ifndef IGR_H_
#define IGR_H_

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#else
#include<random>
#endif
#endif

#include "utility.h"

using namespace std;

class IGR {
private:
    int nvars_;  //subspace size
#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
    unsigned seed_;
#endif

    vector<double> weights_;
    vector<int>    wst_;

    const vector<double>& gain_ratio_vec_;


    int weightedSampling(int random_integer);
    inline vector<int> getRandomWeightedVars();
public:
#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
    IGR(const vector<double>& gain_ratio, int nvars, unsigned seed);
#else
    IGR(const vector<double>& gain_ratio, int nvars);
#endif

    void normalizeWeight(volatile bool* pInterrupt);
    int  getSelectedIdx();
};

#endif /* IGR_H_ */
