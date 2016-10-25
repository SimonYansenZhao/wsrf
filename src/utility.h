#ifndef UTILITY_H_
#define UTILITY_H_

#include <exception>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <Rcpp.h>
#include <random>

using namespace std;

enum ItemType {
    DISCRETE   = 0,
    CONTINUOUS = 1
};
typedef ItemType VarType;

enum NodeType {
    LEAFNODE,
    INTERNALNODE,
    UNKNOWN
};

typedef struct attribute_selection_result {
    bool   ok_;
    int    var_idx_;
    double split_value_;
    double info_gain_;
    double split_info_;
    double gain_ratio_;
    map<int, vector<int> > split_map_;
} VarSelectRes;

class interrupt_exception: public std::exception {
public:
    interrupt_exception(std::string message) :
        detailed_message(message) {
    }

    virtual ~interrupt_exception() throw () {
    }

    virtual const char* what() const throw () {
        return detailed_message.c_str();
    }
    std::string detailed_message;
};

static inline void check_interrupt_impl(void* /*dummy*/) {
    R_CheckUserInterrupt();
}

inline bool check_interrupt() {
    return (R_ToplevelExec(check_interrupt_impl, NULL) == FALSE);
}

const double LN_2 = log((double)2);

// type of prediction
const int PRED_TYPE_NUM        = 5;

const int PRED_TYPE_CLASS_IDX  = 0;
const int PRED_TYPE_VOTE_IDX   = 1;
const int PRED_TYPE_PROB_IDX   = 2;
const int PRED_TYPE_APROB_IDX  = 3;
const int PRED_TYPE_WAPROB_IDX = 4;

const int PRED_TYPE_CLASS  = 1 << PRED_TYPE_CLASS_IDX;   // 1,  0x000001
const int PRED_TYPE_VOTE   = 1 << PRED_TYPE_VOTE_IDX;    // 2,  0x000010
const int PRED_TYPE_PROB   = 1 << PRED_TYPE_PROB_IDX;    // 4,  0x000100
const int PRED_TYPE_APROB  = 1 << PRED_TYPE_APROB_IDX;   // 8,  0x001000
const int PRED_TYPE_WAPROB = 1 << PRED_TYPE_WAPROB_IDX;  // 16, 0x010000



// wsrf$
const int WSRF_MODEL_SIZE          = 19;

const int META_IDX                 = 0;
const int TARGET_DATA_IDX          = 1;
const int TREES_IDX                = 2;
const int TREE_OOB_ERROR_RATES_IDX = 3;
const int OOB_SETS_IDX             = 4;
const int OOB_PREDICT_LABELS_IDX   = 5;
const int TREE_IGR_IMPORTANCE_IDX  = 6;
const int PREDICTED_IDX            = 7;
const int OOB_TIMES_IDX            = 8;
const int CONFUSION_IDX            = 9;
const int IMPORTANCE_IDX           = 10;
const int IMPORTANCESD_IDX         = 11;
const int RF_OOB_ERROR_RATE_IDX    = 12;
const int STRENGTH_IDX             = 13;
const int CORRELATION_IDX          = 14;
const int C_S2_IDX                 = 15;
const int WEIGHTS_IDX              = 16;
const int MTRY_IDX                 = 17;
const int NODESIZE_IDX             = 18;

// targetData$
const string TRAIN_TARGET_LABELS  = "trainTargLabels";
const string NLABELS              = "nlabels";

// meta$
const string NVARS                = "nvars";
const string VAR_NAMES            = "varnames";
const string VAR_TYPES            = "vartypes";
const string VAL_NAMES            = "valnames";

// message
const string INTERRUPT_MSG = "The random forest model building is interrupted.";

#endif
