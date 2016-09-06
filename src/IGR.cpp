#include "IGR.h"

IGR::IGR(const vector<double>& gain_ratio, int nvars, unsigned seed)
    : gain_ratio_vec_(gain_ratio) {

    weights_ = vector<double>(gain_ratio.size()+1);
    wst_     = vector<int>(gain_ratio.size()+1);
    seed_    = seed;

    int n = gain_ratio.size();
    nvars_ = nvars >= n ? n : nvars;
}

int IGR::getSelectedIdx(volatile bool* pInterrupt)
/*
 * select the most weighted variable from < this->m_ > variables that
 * are randomly picked from all variables according to their weights
 */
{
    Sampling rs (seed_);
    const vector<int>& wrs_vec = rs.nonReplaceWeightedSample(gain_ratio_vec_, nvars_, pInterrupt);
    int max = -1;
    bool is_max_set = false;
    for (int rand_num : wrs_vec) {
        if (is_max_set) {
            if (gain_ratio_vec_[rand_num] >= gain_ratio_vec_[max]) max = rand_num;
        } else {
            max = rand_num;
            is_max_set = true;
        }
    }

    return max;
}
