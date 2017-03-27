print.wsrf <- function(x, trees, ...)
{

  ntree <- length(x[[.TREES_IDX]])

  if (missing(trees))
  {
    cat("A Weighted Subspace Random Forest model with ", ntree,
        " tree", ifelse(ntree == 1, "", "s"), ".\n\n", sep="")
    
    cat(sprintf("%38s: %d\n",     "No. of variables tried at each split", x[[.MTRY_IDX]]))
    cat(sprintf("%38s: %d\n",     "Minimum size of terminal nodes",       x[[.NODESIZE_IDX]]))
    cat(sprintf("%38s: %.2f\n",   "Out-of-Bag Error Rate",                x[[.RF_OOB_ERROR_RATE_IDX]]))
    cat(sprintf("%38s: %.2f\n",   "Strength",                             x[[.STRENGTH_IDX]]))
    cat(sprintf("%38s: %.2f\n\n", "Correlation",                          x[[.CORRELATION_IDX]]))
    cat("Confusion matrix:\n")
    print(round(x[[.CONFUSION_IDX]], 2))
  }
  else
  {
    if (is.logical(trees))
      trees <- seq(x[[.TREES_IDX]])[trees]
    .Call(WSRF_print, x, trees)
  }

  invisible()

}



