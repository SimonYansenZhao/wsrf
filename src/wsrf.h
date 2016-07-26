#ifndef WSRF_H
#define WSRF_H

#include "rforest.h"

/*
 * Note : RcppExport is an alias to `extern "C"` defined by Rcpp.
 *
 * It gives C calling convention to the rcpp_hello_world function so that
 * it can be called from .Call in R. Otherwise, the C++ compiler mangles the
 * name of the function and .Call can't find it.
 *
 * It is only useful to use RcppExport when the function is intended to be called
 * by .Call. See the thread http://thread.gmane.org/gmane.comp.lang.r.rcpp/649/focus=672
 * on Rcpp-devel for a misuse of RcppExport.
 */

RcppExport SEXP wsrf (
    SEXP dsSEXP,
    SEXP tnSEXP,
    SEXP ntreesSEXP,
    SEXP nvarsSEXP,
    SEXP weightsSEXP,
    SEXP parallelSEXP,
    SEXP seedsSEXP,
    SEXP importanceSEXP,
    SEXP isPartSEXP);

RcppExport SEXP predict (SEXP wrfSEXP, SEXP dsSEXP, SEXP typeSEXP);
RcppExport SEXP afterReduceForCluster (SEXP wrfSEXP, SEXP dsSEXP, SEXP tnSEXP);
RcppExport SEXP afterMergeOrSubset (SEXP wsrfSEXP);
RcppExport SEXP print (SEXP wsrfSEXP, SEXP treesSEXP);

#endif
