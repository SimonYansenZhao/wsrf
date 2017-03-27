wsrf.default <- function(
    x,
    y,
    mtry=floor(log2(length(x))+1),
    ntree=500,
    weights=TRUE,
    parallel=TRUE,
    na.action=na.fail,
    importance=FALSE,
    nodesize=2,
    clusterlogfile,
    ...) {

  # Perform the required na.action, which defaults to faling if there is missing
  # data in the dataset.

  if (!is.null(na.action)) {
    data <- as.data.frame(na.action(cbind(x, y)))
    x <- data[-length(data)]
    y <- data[[length(data)]]
    rm(data)
  }

  # Prepare to pass execution over to the suitable helper.
  
  if (!is.factor(y))
    y <- as.factor(y)
  
  mtry    <- as.integer(mtry); if (mtry <= 0) stop("mtry should be at least 1.")
  nodesize <- as.integer(nodesize); if (nodesize <= 0) stop("nodesize should be at least 1.")
  ntree  <- as.integer(ntree); if (ntree <= 0) stop("ntree should be at least 1.")
  seeds   <- as.integer(runif(ntree) * 10000000)
  
  # Determine what kind of parallel to perform. By default, when
  # parallel=TRUE, use 2 less than the number of cores available, or 1
  # core if there are only 2 cores.
  
  if (is.logical(parallel) || is.numeric(parallel))
  {
    if (is.logical(parallel) && parallel)
    {
      parallel <- detectCores()-2
      if (is.na(parallel) || parallel < 1) parallel <- 1
    }
    model <- .wsrf(x, y, ntree, mtry, nodesize, weights, parallel, seeds, importance, FALSE)
  }
  else if (is.vector(parallel))
  {
    model <- .clwsrf(x, y, ntree, mtry, nodesize, weights, serverargs=parallel, seeds, importance, clusterlogfile)
  }
  else
    stop ("Parallel must be logical, character, or numeric.")
  
  class(model) <- "wsrf"
  
  return(model)
}



.wsrf <- function(x, y, ntree, mtry, nodesize, weights, parallel, seeds, importance, ispart)
{
  model <- .Call(WSRF_wsrf, x, y, ntree, mtry, nodesize,
      weights, parallel, seeds, importance, ispart)
  names(model) <- .WSRF_MODEL_NAMES
  return(model)
}


.localwsrf <- function(serverargs, x, y, mtry, nodesize, weights, importance)
{
  ntree   <- serverargs[1][[1]]
  parallel <- serverargs[2][[1]]
  seeds    <- serverargs[3][[1]]
  
  model <- .wsrf(x, y, ntree, mtry, nodesize, weights, parallel, seeds, importance, TRUE)
  return(model)
}


.clwsrf <- function(x, y, ntree, mtry, nodesize, weights, serverargs, seeds, importance, clusterlogfile)
{
  # Multiple cores on multiple servers.
  # where serverargs like c("apollo9", "apollo10", "apollo11", "apollo12")
  # or c(apollo9=5, apollo10=8, apollo11=-1)
  
  determineCores <- function()
  {
    if (.Platform$OS.type == "windows") return(1)
    
    nthreads <- detectCores() - 2
    if (nthreads > 0)
      return(nthreads)
    else
      return(1)
  }
  
  if (is.vector(serverargs, "character"))
  {
    nodes <- serverargs
    
    if (missing(clusterlogfile)) cl <- makeCluster(nodes)
    else cl <- makeCluster(nodes, outfile=clusterlogfile)
    
    clusterEvalQ(cl, require(wsrf))
    parallels <- unlist(clusterCall(cl, determineCores))
  }
  else if (is.vector(serverargs, "numeric"))
  {
    nodes <- names(serverargs)
    
    if (missing(clusterlogfile)) cl <- makeCluster(nodes)
    else cl <- makeCluster(nodes, outfile=clusterlogfile)
    
    clusterEvalQ(cl, require(wsrf))
    parallels <- unlist(clusterCall(cl, determineCores))
    parallels <- ifelse(serverargs > 0, serverargs, parallels)
  }
  else
    stop ("Parallel must be a vector of mode character/numeric.")
  
  nservers <- length(nodes)
  
  # just make sure each node has different RNGs in C code, time is
  # part of the seed, so this call won't make a reproducible result
  
  clusterSetRNGStream(cl)
  
  # follow specification in "serverargs", calculate corresponding tree
  # number for each node
  
  nTreesPerNode <- floor(ntree / sum(parallels)) * parallels
  nTreesLeft    <- ntree %% sum(parallels)
  #    cumsumParallels <- cumsum(parallels)
  #    leftPerNode <- ifelse(nTreesLeft >= cumsumParallels, parallels, 0)
  #    if (!(nTreesLeft %in% cumsumParallels)) {
  #        index <- which(nTreesLeft < cumsumParallels)[1]
  #        if (index == 1)
  #            leftPerNode[index] <- nTreesLeft
  #        else
  #            leftPerNode[index] <- nTreesLeft - cumsumParallels[index - 1]
  #    }
  
  ones        <- rep(1, length(parallels))
  leftPerNode <- floor(nTreesLeft / sum(ones)) * ones
  left        <- nTreesLeft %% sum(ones)
  leftPerNode <- leftPerNode + c(rep(1, left), rep(0, length(parallels) - left))
  
  nTreesPerNode <- nTreesPerNode + leftPerNode
  
  parallels <- parallels[which(nTreesPerNode > 0)]
  parallels <- as.integer(parallels)
  
  nTreesPerNode <- nTreesPerNode[which(nTreesPerNode > 0)]
  nTreesPerNode <- as.integer(nTreesPerNode)
  
  seedsPerNode <- split(seeds, rep(1:nservers, nTreesPerNode))
  
  forests <- parRapply(cl, cbind(nTreesPerNode, parallels, seedsPerNode),
      .localwsrf, x, y, mtry, nodesize, weights, importance)
  stopCluster(cl)
  model <- .reduce.wsrf(forests)
  
  # "afterReduceForCluster" is used for statistics.
  .Call(WSRF_afterReduceForCluster, model, x, y)
  
  class(model) <- "wsrf"
  
  return(model)
}

.onAttach <- function(libname, pkgname) {
  wsrfDescription <- "wsrf: An R Package for Scalable Weighted Subspace Random Forests."
  wsrfVersion <- read.dcf(file=system.file("DESCRIPTION", package=pkgname),
      fields="Version")
  
  packageStartupMessage(wsrfDescription)
  packageStartupMessage(paste("Version", wsrfVersion))
  packageStartupMessage("Use C++ standard thread library for parallel computing")
#  packageStartupMessage("With parallel computing disabled")
#  packageStartupMessage("Type wsrfNews() to see new features/changes/bug fixes.")
}


## All the names and indexes of the elements of the model returned by wsrf.

.META                 <- "meta";               .META_IDX                 <- 1;
.TARGET_DATA          <- "targetData";         .TARGET_DATA_IDX          <- 2;
.TREES                <- "trees";              .TREES_IDX                <- 3;
.TREE_OOB_ERROR_RATES <- "treeOOBErrorRates";  .TREE_OOB_ERROR_RATES_IDX <- 4;
.OOB_SETS             <- "OOBSets";            .OOB_SETS_IDX             <- 5;
.OOB_PREDICT_LABELS   <- "OOBPredictLabels";   .OOB_PREDICT_LABELS_IDX   <- 6;
.TREE_IGR_IMPORTANCE  <- "treeIgrImportance";  .TREE_IGR_IMPORTANCE_IDX  <- 7;
.PREDICTED            <- "predicted";          .PREDICTED_IDX            <- 8;
.OOB_TIMES            <- "oob.times";          .OOB_TIMES_IDX            <- 9;
.CONFUSION            <- "confusion";          .CONFUSION_IDX            <- 10;
.IMPORTANCE           <- "importance";         .IMPORTANCE_IDX           <- 11;
.IMPORTANCESD         <- "importanceSD";       .IMPORTANCESD_IDX         <- 12;
.RF_OOB_ERROR_RATE    <- "RFOOBErrorRate";     .RF_OOB_ERROR_RATE_IDX    <- 13;
.STRENGTH             <- "strength";           .STRENGTH_IDX             <- 14;
.CORRELATION          <- "correlation";        .CORRELATION_IDX          <- 15;
.C_S2                 <- "c_s2";               .C_S2_IDX                 <- 16;
.WEIGHTS              <- "useweights";         .WEIGHTS_IDX              <- 17;
.MTRY                 <- "mtry";               .MTRY_IDX                 <- 18;
.NODESIZE             <- "nodesize";           .NODESIZE_IDX             <- 19;

.WSRF_MODEL_SIZE      <- 19
.WSRF_MODEL_NAMES     <- c(
    .META,
    .TARGET_DATA,
    .TREES,
    .TREE_OOB_ERROR_RATES,
    .OOB_SETS,
    .OOB_PREDICT_LABELS,
    .TREE_IGR_IMPORTANCE,
    .PREDICTED,
    .OOB_TIMES,
    .CONFUSION,
    .IMPORTANCE,
    .IMPORTANCESD,
    .RF_OOB_ERROR_RATE,
    .STRENGTH,
    .CORRELATION,
    .C_S2,
    .WEIGHTS,
    .MTRY,
    .NODESIZE)


