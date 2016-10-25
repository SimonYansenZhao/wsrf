#include "wsrf.h"

#include <thread>
#include <chrono>
#include <future>


using namespace std;

SEXP wsrf (
    SEXP xSEXP,         // Data.
    SEXP ySEXP,         // Target variable name.
    SEXP ntreeSEXP,     // Number of trees.
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


        RForest rf (&train_set, &targ_data, &meta_data,
                    Rcpp::as<int>(ntreeSEXP), Rcpp::as<int>(nvarsSEXP), Rcpp::as<int>(minnodeSEXP), Rcpp::as<bool>(weightsSEXP),
                    Rcpp::as<bool>(importanceSEXP), seedsSEXP);

        volatile bool interrupt = false;


        int nthreads       = Rcpp::as<int>(parallelSEXP);
        int nCoresMinusTwo = thread::hardware_concurrency() - 2;
        if (nthreads == 0 || nthreads == 1 || (nthreads < 0 && nCoresMinusTwo == 1)) {  // build trees sequentially

            rf.buidForestSeq(&interrupt);

        } else {  // run in parallel

            /**
             * create a thread for model building
             * leave main thread for interrupt check once per 100 milliseconds
             *
             * <interrupt> is used to inform each thread of no need to continue, but has 2 roles:
             *  1. represents user interrupt
             *  2. represents a exception has been thrown from one tree builder
             */

            future<void> res = async(launch::async, &RForest::buildForestAsync, &rf, nthreads, &interrupt);
            try {

                while (true) {
//                  this_thread::sleep_for(chrono::seconds {1});

                    // check interruption
                    if (check_interrupt()) {
                        interrupt = true;
                        throw interrupt_exception(INTERRUPT_MSG);
                    }

                    // check RF thread completion

#if (defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ >= 5))) || defined(__clang__)
                    if (res.wait_for(chrono::milliseconds {100}) == future_status::ready) {
#else  // #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
                    if (res.wait_for(chrono::milliseconds {100})) {
#endif // #if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
                        res.get();
                        break;
                    } // if ()
                } // while (true)

            } catch (...) {

                // make sure thread is finished
                if (res.valid())
                    res.get();

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
