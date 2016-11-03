#include "sampling.h"

Sampling::Sampling(unsigned seed, volatile bool* pInterrupt, bool isParallel) {
    seed_ = seed;
    pInterrupt_ = pInterrupt;
    isParallel_ = isParallel;
}

vector<int> Sampling::nonReplaceRandomSample(vector<int> var_vec, int nselect)
/*
 * Randomly sample <nselect> numbers from <var_vec> without replacement.
 * If <nselect> greater than the size of <var_vec>, return <var_vec> itself.
 */
{
    int nleft = var_vec.size();
    if (nselect >= nleft) return var_vec;

    vector<int> result(nselect);

    default_random_engine re {seed_};
    for (int i = 0; i < nselect; ++i) {

        uniform_int_distribution<int> uid {0, nleft - 1};
        int random_num = uid(re);

        result[i] = var_vec[random_num];
        var_vec[random_num] = var_vec[nleft - 1];
        nleft--;

    }

    return result;
}

vector<int> Sampling::nonReplaceWeightedSample(const vector<double>& originalweights, int nselect, bool needsqrt)
/*
 * Weighted randomly sample without replacement.
 * Return the indexes of items in <originalweights> being selected.
 */
{
    int n    = originalweights.size();
    nselect  = nselect >= n ? n : nselect;
    weights_ = vector<double>(n+1);
    wst_     = vector<int>(n+1);

    vector<int> result(nselect);

    /*
     * If the number of selection is greater than the size of the vector, then return all.
     */
    if (nselect >= n) {
        for (int i = 0; i < n; i++)
            result[i] = i;
        return result;
    }

    /*
     * Normalize weights.
     */
    double sum = 0;
    for (int i = 0, j = 1; i < n; i++, j++) {

        if (!isParallel_ && check_interrupt()) {
            // If run sequentially, check user interruption directly.
            throw interrupt_exception(MODEL_INTERRUPT_MSG);
        } else if (*pInterrupt_) {
            return vector<int>();
        }

        weights_[j] = needsqrt ? sqrt(originalweights[i]) : originalweights[i];
        sum += weights_[j];
    }

    if (sum != 0) {
        for (int i = 1; i <= n; i++) {
            weights_[i] /= sum;
            int temp = weights_[i] * RAND_MAX;
            weights_[i] = temp;
        }
    } else {
        int temp = RAND_MAX / (double) n;
        for (int i = 1; i <= n; i++) {
            weights_[i] = temp;
        }
    }

    /*
     * Weighted sampling.
     */
    copy(weights_.begin(), weights_.end(), wst_.begin());
    for (int i = n; i > 1; i--) wst_[i>>1] += wst_[i];

    default_random_engine re {seed_};

    for(int i = 0; i < nselect; ++ i) {
        uniform_int_distribution<int> uid {0, wst_[1]-1};
        int rand_num = uid(re);
        int j = 1;
        while (rand_num > weights_[j]) {
            rand_num -= weights_[j];
            j <<= 1;
            if (rand_num > wst_[j]) {
                rand_num -= wst_[j];
                j++;
            }
        }

        result[i] = j-1;

        int w = weights_[j];
        weights_[j] = 0;

        while (j != 0) {
            wst_[j] -= w;
            j >>= 1;
        }
    //    //may be the largest right is smaller RAND_MAX,because double to int may lose information
    //    return n - 1;
    }

    return result;
}

vector<int> Sampling::nonReplaceWeightedSample(const vector<int>& var_vec, const vector<double>& originalweights, int nselect, bool needsqrt) {
    vector<int> res = nonReplaceWeightedSample(originalweights, nselect, needsqrt);
    for (int i = 0; i < (int)res.size(); i++)
        res[i] = var_vec[res[i]];
    return res;
}
