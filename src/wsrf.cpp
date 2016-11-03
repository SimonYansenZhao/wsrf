#include "wsrf.h"

#include <thread>
#include <chrono>
#include <future>


using namespace std;

SEXP wsrf (
    SEXP xSEXP,          // Data.
    SEXP ySEXP,          // Target variable name.
    SEXP ntreeSEXP,      // Number of trees.
    SEXP nvarsSEXP,      // Number of variables.
    SEXP minnodeSEXP,    // Minimum node size.
    SEXP weightsSEXP,    // Whether use weights.
    SEXP parallelSEXP,   // Whether parallel or how many cores performing parallelism.
    SEXP seedsSEXP,      // Random seeds for each trees.
    SEXP importanceSEXP, // Whether calculate variable importance measures.
    SEXP ispartSEXP      // Indicating whether it is part of the whole forests.
    )
/*
 * Main entry function for building random forests model.
 */
{
    BEGIN_RCPP

        MetaData   meta_data (xSEXP, ySEXP);
        TargetData targ_data (ySEXP);
        Dataset    train_set (xSEXP, &meta_data, true);

        /*
         * <interrupt> is used to inform each thread of no need to continue, but has 2 roles:
         *  1. represents user interrupt.
         *  2. represents a exception has been thrown from one tree builder.
         *
         * Interruption check points:
         *  1. check at the beginning of every tree growing.
         *  2. check at the beginning of every node growing.
         *  3. check at the beginning of time consuming operation.
         */

        volatile bool interrupt = false;

        RForest rf (&train_set, &targ_data, &meta_data,
                    Rcpp::as<int>(ntreeSEXP), Rcpp::as<int>(nvarsSEXP), Rcpp::as<int>(minnodeSEXP), Rcpp::as<bool>(weightsSEXP),
                    Rcpp::as<bool>(importanceSEXP), seedsSEXP, &interrupt);



        int nthreads       = Rcpp::as<int>(parallelSEXP);
        int nCoresMinusTwo = thread::hardware_concurrency() - 2;
        if (nthreads == 0 || nthreads == 1 || (nthreads < 0 && nCoresMinusTwo == 1)) {

            rf.buidForestSeq();  // Grow trees sequentially

        } else {

            // Create a thread for model building and leave main thread for interrupt check.

            future<void> res = async(launch::async, &RForest::buildForestAsync, &rf, nthreads);
            try {

                while (true) {

                    // check interruption per 100 milliseconds.
                    this_thread::sleep_for(chrono::milliseconds {100});
                    if (check_interrupt()) {
                        interrupt = true;
                        throw interrupt_exception(MODEL_INTERRUPT_MSG);
                    }

//                    interrupt = true;
//                    throw interrupt_exception(MODEL_INTERRUPT_MSG);

                    // check RF thread completion

#if (defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ >= 5))) || defined(__clang__)
                    if (res.valid() && res.wait_for(chrono::milliseconds {0}) == future_status::ready) {
#else  // #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
                    if (res.valid() && res.wait_for(chrono::milliseconds {0})) {
#endif // #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
                        res.get();  // May throw exception.
                        break;
                    } // if ()
                } // while (true)

            } catch (...) {  // Interrupted or exception from sub-thread.

                // Make sure sub-thread is finished if interrupted.
                if (res.valid()) {
                    res.wait();
                }

                rethrow_exception(current_exception());

            } // try-catch

        } // if-else

        Rcpp::List wsrf_R(WSRF_MODEL_SIZE);

        if (!Rcpp::as<bool>(ispartSEXP)) {
            rf.calcEvalMeasures();
            wsrf_R[META_IDX]        = meta_data.save();
            wsrf_R[TARGET_DATA_IDX] = targ_data.save();
            rf.saveMeasures(wsrf_R);
        }

        rf.saveModel(wsrf_R);  // Should be called after RForest::calcEvalMeasures().

        return wsrf_R;

    END_RCPP
}

SEXP afterReduceForCluster (SEXP wsrfSEXP, SEXP xSEXP, SEXP ySEXP) {
    BEGIN_RCPP

        Rcpp::List      wsrf_R    (wsrfSEXP);
        MetaData        meta_data (xSEXP, ySEXP);
        TargetData      targ_data (ySEXP);
        RForest         rf        (wsrf_R, &meta_data, &targ_data);

        rf.calcEvalMeasures();

        wsrf_R[META_IDX]        = meta_data.save();
        wsrf_R[TARGET_DATA_IDX] = targ_data.save();

        rf.saveMeasures(wsrf_R);  // Should be called after RForest::calcEvalMeasures().

    END_RCPP
}

SEXP afterMergeOrSubset (SEXP wsrfSEXP) {
    BEGIN_RCPP

        Rcpp::List wsrf_R    (wsrfSEXP);
        MetaData   meta_data (Rcpp::as<Rcpp::List>((SEXPREC*)wsrf_R[META_IDX]));
        TargetData targ_data (Rcpp::as<Rcpp::List>((SEXPREC*)wsrf_R[TARGET_DATA_IDX]));
        RForest    rf        (wsrf_R, &meta_data, &targ_data);

        rf.calcEvalMeasures();

        rf.saveMeasures(wsrf_R);  // Should be called after RForest::calcEvalMeasures().

    END_RCPP
}

SEXP predict (SEXP wsrfSEXP, SEXP xSEXP, SEXP typeSEXP) {

    BEGIN_RCPP

        Rcpp::List wsrf_R    (wsrfSEXP);
        MetaData   meta_data (Rcpp::as<Rcpp::List>((SEXPREC*)wsrf_R[META_IDX]));
        Dataset    test_set  (xSEXP, &meta_data, false);
        RForest    rf        (wsrf_R, &meta_data, NULL);

        int type = Rcpp::as<int>(typeSEXP);
        return rf.predict(&test_set, type);

    END_RCPP
}

SEXP print (SEXP wsrfSEXP, SEXP treesSEXP) {
    BEGIN_RCPP

        Rcpp::List          wsrf_R            (wsrfSEXP);
        MetaData            meta_data         (Rcpp::as<Rcpp::List>((SEXPREC*)wsrf_R[META_IDX]));
        Rcpp::List          trees             (wsrf_R[TREES_IDX]);
        Rcpp::NumericVector tree_error_rates  (wsrf_R[TREE_OOB_ERROR_RATES_IDX]);
        Rcpp::IntegerVector tree_idx_vec      (treesSEXP);

        int n = tree_idx_vec.size();
        for (int i = 0; i < n; i++) {
            int    index      = tree_idx_vec[i]-1;
            double error_rate = tree_error_rates[index];

            vector<vector<double> > node_infos = Rcpp::as<vector<vector<double> > >((SEXPREC*)trees[index]);

            int ntests = 0;
            int nnodes = node_infos.size();
            for (int k = 0; k < nnodes; k++)
                if (node_infos[k][0] == INTERNALNODE)
                    ntests++;

            if (i > 0) Rprintf("======================================================================\n");
            Rprintf("Tree %d has %d tests (internal nodes), with OOB error rate %.4f:\n\n", index+1, ntests, error_rate);

            Tree tree (node_infos, &meta_data, error_rate);
            tree.print();
        }

    END_RCPP
}
