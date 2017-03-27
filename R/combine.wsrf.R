.reduce.wsrf <- function(xs, ...)
{
  ## Reduce multiple models of wsrf into one.

  # xs should be a list of objects of wsrf.

  tags <- c(.TREES_IDX, .TREE_OOB_ERROR_RATES_IDX, .OOB_SETS_IDX, .OOB_PREDICT_LABELS_IDX, .TREE_IGR_IMPORTANCE_IDX, .WEIGHTS_IDX, .MTRY_IDX, .NODESIZE_IDX)

  res <- vector("list", .WSRF_MODEL_SIZE)
  names(res) <- .WSRF_MODEL_NAMES

  for (tag in tags)
    res[[tag]] <- unlist(lapply(xs, function(x, tg) { x[[tg]] }, tag), recursive=FALSE, use.names=FALSE)

  for (tag in c(.WEIGHTS_IDX, .MTRY_IDX, .NODESIZE_IDX)) {
    if (!is.null(res[[tag]]) && length(unique(res[[tag]]))==1) res[[tag]] <- res[[tag]][1]
    else res[tag] <- list(NULL)
  }

  if (!is.null(xs[[1]][[.META_IDX]]))
    res[[.META_IDX]] <- xs[[1]][[.META_IDX]]

  if (!is.null(xs[[1]][[.TARGET_DATA_IDX]]))
    res[[.TARGET_DATA_IDX]] <- xs[[1]][[.TARGET_DATA_IDX]]

  return(res)
}

combine <- function(...) UseMethod("combine")
combine.wsrf <- function(...)
{
  ## Merge ... into one bigger model of wsrf.

  res <- list(...)

  areWsrfObjects <- sapply(res, function(x) inherits(x, "wsrf")) 
  if (any(!areWsrfObjects)) stop("Argument must be a list of wsrf objects")

  res <- .reduce.wsrf(res)

  .Call(WSRF_afterMergeOrSubset, res)

  class(res) <- "wsrf"

  return(res)
}
