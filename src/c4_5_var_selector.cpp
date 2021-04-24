#include "c4_5_var_selector.h"

C4p5Selector::C4p5Selector (
        Dataset* train_set,
        TargetData* targdata,
        MetaData* meta_data,
        int min_node_size,
        const vector<int>& obs_vec,
        const vector<int>& var_vec,
        int mtry,
        unsigned seed,
        volatile bool* pInterrupt,
        bool isParallel)
    : VarSelector(train_set, targdata, meta_data, obs_vec, var_vec) {
    seed_ = seed;
    info_ = calcEntropy(obs_vec);
    mtry_ = mtry;
    min_node_size_ = min_node_size;
    pInterrupt_ = pInterrupt;
    isParallel_ = isParallel;

}

void C4p5Selector::handleDiscVar (int var_idx)
/*
 * Calculate corresponding information if split by discrete variable <var_idx>.
 *
 * If no more than 2 child nodes contain at least <MIN_NODE_SIZE_>
 * instances, don't split training set by this attribute
 */
{
    map<int, vector<int> > mapper = train_set_->splitDiscVar(obs_vec_, var_idx);
    int count = 0;
    for (map<int, vector<int> >::iterator iter = mapper.begin(); iter != mapper.end(); ++iter)
        if (int(iter->second.size()) >= min_node_size_) count++;

    if (count < 2) return;

    /*
     * calculate corresponding information gain and split entropy
     * while split by this variable <var_idx>
     */
    double subinfo = 0;
    double split_info = 0;
    for (map<int, vector<int> >::iterator iter = mapper.begin(); iter != mapper.end(); ++iter) {
        int nobs_sub = iter->second.size();
        if (nobs_sub != 0) {
            split_info += train_set_->nlogn(nobs_sub);
            subinfo += sumNlogn(targ_data_->getLabelFreqCount(iter->second), nobs_sub);
        }
    }

    double info_gain = info_ - subinfo/nobs_;
    if (info_gain <= 0) return;

    split_info = (train_set_->nlogn(nobs_) -  split_info) / nobs_;

    cand_splits_map_[var_idx].swap(mapper);
    info_gain_map_[var_idx] = info_gain;
    split_info_map_[var_idx] = split_info;
}

template<class T>
void C4p5Selector::handleContVar (int var_idx) {
    //TODO: Need better way to deal with different type of variable, that is DISCRETE, INTSXP, REALSXP.
    if (nobs_ < 2 * min_node_size_) return;

    vector<int> sorted_obs_vec = obs_vec_;
    sort(sorted_obs_vec.begin(), sorted_obs_vec.end(), VarValueComparor<T>(train_set_, var_idx));

    vector<int> left_dstr(meta_data_->nlabels(), 0);
    vector<int> right_dstr = targ_data_->getLabelFreqCount(sorted_obs_vec);


    int current_label = -1;
    for (int i = 0; i < min_node_size_; ++i) {
        current_label = targ_data_->getLabel(sorted_obs_vec[i]) - 1;
        left_dstr[current_label]++;
        right_dstr[current_label]--;
    }

    T* var_array = train_set_->getVar<T>(var_idx);
    double current_value = var_array[sorted_obs_vec[min_node_size_-1]];
    double subinfo;
    double split_value = -1;
    bool subinfo_is_set = false;
    int pos = min_node_size_ - 1;
    for (int i = min_node_size_; i < nobs_ - min_node_size_; ++i) {
        int next_label = targ_data_->getLabel(sorted_obs_vec[i]) - 1;
        double next_value = var_array[sorted_obs_vec[i]];
        if (current_label != next_label && current_value != next_value) {
            double new_subinfo = calcBisectSubinfo(left_dstr, i, right_dstr, nobs_ - i);
            if (subinfo_is_set) {
                if (new_subinfo < subinfo) {
                    subinfo = new_subinfo;
                    split_value = current_value;
                    pos = i - 1;
                }
            } else {
                subinfo = new_subinfo;
                split_value = current_value;
                subinfo_is_set = true;
                pos = i - 1;
            }

        }
        left_dstr[next_label]++;
        right_dstr[next_label]--;
        current_label = next_label;
        current_value = next_value;
    }

    if (subinfo_is_set) {
        double info_gain = info_ - subinfo;
        if (info_gain <= 0) return;

        info_gain_map_[var_idx] = info_gain;

//        T* vararray = (T *) ((*train_set_)[var_idx]);
//        double split_value = (vararray[sorted_obs_vec[pos]] + vararray[sorted_obs_vec[pos + 1]]) / 2;
        double split_info = (train_set_->nlogn(nobs_) - train_set_->nlogn(pos + 1) - train_set_->nlogn(nobs_ - pos - 1)) / nobs_;
        split_info_map_[var_idx] = split_info;

        map<int, vector<int> > mapper = train_set_->splitPosition(sorted_obs_vec, pos);
        cand_splits_map_[var_idx].swap(mapper);

        split_value_map_[var_idx] = split_value;
    }
}

void C4p5Selector::handleContVar (int var_idx)
/*
 * Calculate corresponding information if split by numerical variable <var_idx>.
 */
{
    switch (meta_data_->getVarType(var_idx)) {
    case INTSXP:
        handleContVar<int>(var_idx);
        break;
    case REALSXP:
        handleContVar<double>(var_idx);
        break;
    default:
        throw std::range_error(meta_data_->getVarName(var_idx) + UNEXPECTED_VAR_TYPE_MSG);
    };
}

void C4p5Selector::calcInfos (const vector<int>& var_vec)
/*
 * Calculate the impurity difference when using each variable for node splitting.
 */
{
    int n = var_vec.size();
    for (int i = 0; i < n; i++) {


        if (!isParallel_ && check_interrupt()) {
            // If run sequentially, check user interruption directly.
            throw interrupt_exception(MODEL_INTERRUPT_MSG);
        } else if (*pInterrupt_) {
            // Otherwise, return immediately if run in parallel.
            return;
        }

        if (meta_data_->getVarType(var_vec[i]) == DISCRETE) {
            handleDiscVar(var_vec[i]);
        } else {
            handleContVar(var_vec[i]);
        }
    }
}

double C4p5Selector::averageInfoGain () {
    double total_info_gain = 0;
    for (map<int, double>::iterator iter = info_gain_map_.begin(); iter != info_gain_map_.end(); ++iter)
        total_info_gain += iter->second;

    // the average_info_gain minus 0.001 to avoid the situation where all the info gain is the same
    double average_info_gain = total_info_gain / (double) ((info_gain_map_.size())) - 0.000001;
    return average_info_gain;
}

void C4p5Selector::doIGRSelection (VarSelectRes& res)
/*
 * calculate all information gain when split by any one of the variables
 * from the weighted randomly selected subspace of size <mtry_>
 */
{
    calcInfos(var_vec_);

    if (!isParallel_ && check_interrupt()) {
        // If run sequentially, check user interruption directly.
        throw interrupt_exception(MODEL_INTERRUPT_MSG);
    } else if (info_gain_map_.empty() || *pInterrupt_) {
        setResult(-1, res);
        return;
    }

    double average_info_gain = averageInfoGain();

    /*
     * following code is IGR weighting method
     */
    double gain_ratio;
    vector<int> cand_var_vec;
    vector<double> cand_gain_ratio_vec;
    for (map<int, double>::iterator iter = info_gain_map_.begin(); iter != info_gain_map_.end(); ++iter) {
        if (iter->second >= average_info_gain) {
            double split_info = split_info_map_[iter->first];
            if (split_info > 0) {
                gain_ratio = iter->second / split_info;
                cand_var_vec.push_back(iter->first);
                cand_gain_ratio_vec.push_back(gain_ratio);
            }
        }
    }

    if (!isParallel_ && check_interrupt()) {
        // If run sequentially, check user interruption directly.
        throw interrupt_exception(MODEL_INTERRUPT_MSG);
    } else if (*pInterrupt_) {
        setResult(-1, res);
        return;
    }

    int vindex;
    if (cand_var_vec.size() == 0) {
        vindex = info_gain_map_.begin()->first;
        double split_info = split_info_map_[vindex];

        gain_ratio = split_info > 0 ? info_gain_map_.begin()->second / split_info : NA_REAL;
    } else {

        IGR igr(cand_gain_ratio_vec, mtry_, seed_, pInterrupt_, isParallel_);
        int index = igr.getSelectedIdx();

        if (!isParallel_ && check_interrupt()) {
            // If run sequentially, check user interruption directly.
            throw interrupt_exception(MODEL_INTERRUPT_MSG);
        } else if (*pInterrupt_) {
            setResult(-1, res);
            return;
        }

        vindex = cand_var_vec[index];

        gain_ratio = cand_gain_ratio_vec[index];
    }

    setResult(vindex, res, gain_ratio);
}

void C4p5Selector::setResult (int vindex, VarSelectRes& result, double gain_ratio) {
    if (vindex >= 0) {
        result.ok_          = true;
        result.var_idx_     = vindex;
        result.split_value_ = split_value_map_[vindex];
        result.info_gain_   = info_gain_map_[vindex];
        result.split_info_  = split_info_map_[vindex];
        result.gain_ratio_  = gain_ratio;
        result.split_map_.swap(cand_splits_map_[vindex]);
    } else {
        result.ok_ = false;
    }
}

void C4p5Selector::doSelection (VarSelectRes& res)
/*
 * calculate all information gain when split by any one of the variables
 * from the randomly selected subspace of size <mtry_>
 */
{
    Sampling rs (seed_, pInterrupt_, isParallel_);
    vector<int> subvar_vec = rs.nonReplaceRandomSample(var_vec_, mtry_);

    calcInfos(subvar_vec);

    if (!isParallel_ && check_interrupt()) {
        // If run sequentially, check user interruption directly.
        throw interrupt_exception(MODEL_INTERRUPT_MSG);
    } else if (info_gain_map_.empty() || *pInterrupt_) {
        setResult(-1, res);
        return;
    }

    findBest(res);
}

void C4p5Selector::findBest(VarSelectRes& res)
/*
 * find the best variable.
 */
{
    double average_info_gain = averageInfoGain();
    double gain_ratio = -1;
    int vindex = -1;
    bool is_set_gain_ratio = false;
    for (map<int, double>::iterator iter = info_gain_map_.begin(); iter != info_gain_map_.end(); ++iter) {

        if (!isParallel_ && check_interrupt()) {
            // If run sequentially, check user interruption directly.
            throw interrupt_exception(MODEL_INTERRUPT_MSG);
        } else if (*pInterrupt_) {
            setResult(-1, res);
            return;
        }

        if (iter->second >= average_info_gain) {
            double split_info = split_info_map_[iter->first];
            if (split_info > 0) {
                double new_gain_ratio = iter->second / split_info;
                if (is_set_gain_ratio) {
                    if (new_gain_ratio > gain_ratio) {
                        gain_ratio = new_gain_ratio;
                        vindex     = iter->first;
                    }
                } else {
                    gain_ratio = new_gain_ratio;
                    vindex     = iter->first;
                    is_set_gain_ratio = true;
                }
            }
        }
    }

    if (!is_set_gain_ratio) {
        vindex     = info_gain_map_.begin()->first;
        double split_info = split_info_map_[vindex];
        gain_ratio = split_info > 0 ? info_gain_map_.begin()->second / split_info : NA_REAL;
    }

    setResult(vindex, res, gain_ratio);
}

