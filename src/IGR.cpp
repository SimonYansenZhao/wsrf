#include "IGR.h"

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
IGR::IGR(const vector<double>& gain_ratio, int nvars, unsigned seed)
#else
IGR::IGR(const vector<double>& gain_ratio, int nvars)
#endif
    : gain_ratio_vec_(gain_ratio) {

    weights_ = vector<double>(gain_ratio.size()+1);
    wst_     = vector<int>(gain_ratio.size()+1);
#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
    seed_    = seed;
#endif

    int n = gain_ratio.size();
    if (nvars == -1) nvars = log((double)n) / LN_2 + 1;
    nvars_ = nvars >= n ? n : nvars;
}

int IGR::weightedSampling(int rand_num) {
    int i = 1;
    while (rand_num > weights_[i]) {
        rand_num -= weights_[i];
        i <<= 1;
        if (rand_num > wst_[i]) {
            rand_num -= wst_[i];
            i++;
        }
    }

    int res = i-1;
    int w = weights_[i];
    weights_[i] = 0;

    while (i != 0) {
        wst_[i] -= w;
        i >>= 1;
    }

    return res;

//    //may be the largest right is smaller RAND_MAX,because double to int may lose information
//    return n - 1;
}

/*
 * generate an integer list of size <size> according to probability
 * that is, select <size> variables by their weights
 */
vector<int> IGR::getRandomWeightedVars() {

    //TODO: If possible, make similar RNG codes into a single function.

    int n = weights_.size()-1;
    vector<int> result(nvars_ >= n ? n : nvars_);

    if (nvars_ >= n) {
        for (int i = 0; i < n; i++)
            result[i] = i;
        return result;
    }

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
    boost::random::mt19937 re(seed_);
#else
    default_random_engine re {seed_};
#endif
#else
    Rcpp::RNGScope rngScope;
#endif

    for(int i = 0; i < nvars_; ++ i) {

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
#ifdef WSRF_USE_BOOST
        boost::random::uniform_int_distribution<int> uid(0, wst_[1]-1);
#else
        uniform_int_distribution<int> uid {0, wst_[1]-1};
#endif
        result[i] = weightedSampling(uid(re));
#else
        result[i] = weightedSampling(unif_rand() * wst_[1]);
#endif

    }

    return result;
}

/*
 * calculate weights of all variables according to their gain ratios
 * the results are in this->weights_
 */
void IGR::normalizeWeight(volatile bool* pInterrupt) {

    double sum = 0;
    int n = gain_ratio_vec_.size();

    for (int i = 0, j = 1; i < n; i++, j++) {

#if defined WSRF_USE_BOOST || defined WSRF_USE_C11
        if (*pInterrupt)
            return;
#else
        // check interruption
        if (check_interrupt()) throw interrupt_exception("The random forest model building is interrupted.");
#endif

        weights_[j] = sqrt(gain_ratio_vec_[i]);
        sum += weights_[j];
    }

    if (sum != 0) {
        for (int i = 1; i <= n; i++) {
            weights_[i] /= sum;
            int temp = weights_[i] * RAND_MAX;
            weights_[i] = temp;
            wst_[i] = temp;
        }
    } else {
        int temp = RAND_MAX / (double) n;
        for (int i = 1; i <= n; i++) {
            weights_[i] = temp;
            wst_[i] = temp;
        }
    }

    for (int i = n; i > 1; i--) {
        wst_[i>>1] += wst_[i];
    }
}

/*
 * select the most weighted variable from < this->m_ > variables that
 * are randomly picked from all varialbes according to their weights
 */
int IGR::getSelectedIdx() {
    const vector<int>& wrs_vec = getRandomWeightedVars();
    int max = -1;
    bool is_max_set = false;
    for (int i = 0, rand_num; i < nvars_; i++) {
        rand_num = wrs_vec[i];
        if (is_max_set) {
            if (gain_ratio_vec_[rand_num] >= gain_ratio_vec_[max]) max = rand_num;
        } else {
            max = rand_num;
            is_max_set = true;
        }
    }

    return max;
}
