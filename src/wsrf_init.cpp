#include <stdlib.h> // for NULL
#include <R_ext/Rdynload.h>

#include "wsrf.h"  // Include the header file for declarations of the functions.

#define CALLDEF(name, n) {#name, (DL_FUNC) &name, n}

static const R_CallMethodDef callEntries[] = {
    CALLDEF(wsrf, 10),
    CALLDEF(print, 2),
    CALLDEF(predict, 3),
    CALLDEF(afterReduceForCluster, 3),
    CALLDEF(afterMergeOrSubset, 1),
    {NULL, NULL, 0}
};


RcppExport void R_init_wsrf(DllInfo *dll) {
    R_registerRoutines(dll, NULL, callEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}

