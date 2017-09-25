\name{wsrf}

\alias{wsrf}
\alias{wsrf.formula}
\alias{wsrf.default}

\concept{weighted subspace decision trees}
\concept{weighted subspace random forest}

\title{

  Build a Forest of Weighted Subspace Decision Trees

}

\description{

  Build weighted subspace C4.5-based decision trees to construct a
  forest.

}

\usage{

\method{wsrf}{formula}(formula, data, ...)
\method{wsrf}{default}(x, y, mtry=floor(log2(length(x))+1), ntree=500,
                       weights=TRUE, parallel=TRUE, na.action=na.fail,
                       importance=FALSE, nodesize=2, clusterlogfile, ...)

}

\arguments{

  \item{x, formula}{a data frame or a matrix of predictors, or a formula
    with a response but no interaction terms.}

  \item{y}{a response vector.}

  \item{data}{a data frame in which to interpret the variables named in
    the formula.}

  \item{ntree}{number of trees to grow.  By default, 500}

  \item{mtry}{number of variables to choose as candidates at each node
    split, by default, \code{floor(log2(length(x))+1)}.}

  \item{weights}{logical.  \code{TRUE} for weighted subspace selection,
    which is the default; \code{FALSE} for random selection, and the
    trees are based on C4.5.}

  \item{na.action}{a function indicate the behaviour when encountering
    NA values in \code{data}.  By default, \code{na.fail}.  If
    \code{NULL}, do nothing.}

  \item{parallel}{whether to run multiple cores (TRUE), nodes, or
    sequentially (FALSE).}

  \item{importance}{should importance of predictors be assessed? }

  \item{nodesize}{minimum size of leaf node, i.e., minimum number of
      observations a leaf node represents.  By default, 2.}

  \item{clusterlogfile}{character.  The pathname of the log file when
      building model in a cluster.  For debug.}

  \item{...}{optional parameters to be passed to the low level function
             \code{wsrf.default}.}
  
}

\details{

  See Xu, Huang, Williams, Wang, and Ye (2012) for more details of the
  algorithm, and Zhao, Williams, Huang (2017) for more details of the
  package.

  Currently, \pkg{wsrf} can only be used for classification.  When
  \code{weights=FALSE}, C4.5-based trees (Quinlan (1993)) are grown by
  \code{wsrf}, where binary split is used for continuous predictors
  (variables) and \emph{k}-way split for categorical ones.  For
  continuous predictors, each of the values themselves is used as split
  points, no discretization used.  The only stopping condition for split
  is the minimum node size must not less than \code{nodesize}.

}

\value{

  An object of class \pkg{wsrf}, which is a list with the following
  components:

  \item{confusion}{the confusion matrix of the prediction (based on OOB
  data).}

  \item{oob.times}{number of times cases are `out-of-bag' (and thus used
  in computing OOB error estimate)}

  \item{predicted}{the predicted values of the input data based on
    out-of-bag samples.}

  \item{useweights}{logical.  Whether weighted subspace selcetion is
  used?  NULL if the model is obtained by combining multiple \pkg{wsrf}
  model and one of them has different value of 'useweights'.}

  \item{mtry}{integer.  The number of variables to be chosen when
    spliting a node.}
}

\examples{
  library("wsrf")

  # Prepare parameters.
  ds <- rattle.data::weather
  dim(ds)
  names(ds)
  target <- "RainTomorrow"
  id     <- c("Date", "Location")
  risk   <- "RISK_MM"
  ignore <- c(id, if (exists("risk")) risk) 
  vars   <- setdiff(names(ds), ignore)
  if (sum(is.na(ds[vars]))) ds[vars] <- randomForest::na.roughfix(ds[vars])
  ds[target] <- as.factor(ds[[target]])
  (tt  <- table(ds[target]))
  form <- as.formula(paste(target, "~ ."))
  set.seed(42)
  train <- sample(nrow(ds), 0.7*nrow(ds))
  test  <- setdiff(seq_len(nrow(ds)), train)

  # Build model.  We disable parallelism here, since CRAN Repository
  # Policy (https://cran.r-project.org/web/packages/policies.html)
  # limits the usage of multiple cores to save the limited resource of
  # the check farm.

  model.wsrf <- wsrf(form, data=ds[train, vars], parallel=FALSE)
  
  # View model.
  print(model.wsrf)
  print(model.wsrf, tree=1)

  # Evaluate.
  strength(model.wsrf)
  correlation(model.wsrf)
  res <- predict(model.wsrf, newdata=ds[test, vars], type=c("response", "waprob"))
  actual <- ds[test, target]
  (accuracy.wsrf <- mean(res$response==actual))
  
  # Different type of prediction.
  cl <- apply(res$waprob, 1, which.max)
  cl <- factor(cl, levels=1:ncol(res$waprob), labels=levels(actual))
  (accuracy2.wsrf <- mean(cl==actual))
}

\references{

  Xu, B. and Huang, J. Z. and Williams, G. J. and Wang, Q. and Ye, Y.
  2012
  "Classifying very high-dimensional data with random forests built from small subspaces".
  \emph{International Journal of Data Warehousing and Mining (IJDWM)},
  \bold{8(2)}, 44--63.

  Quinlan, J. R.  1993 \emph{C4.5: Programs for Machine Learning}.
  Morgan Kaufmann.

  Zhao, H. and Williams, G. J. and Huang, J. Z.  2017
  "wsrf: An R Package for Classification with Scalable Weighted Subspace Random Forests".
  \emph{Journal of Statistical Software}, \bold{77(3)}, 1--30.
  doi:10.18637/jss.v077.i03

}

\author{

  He Zhao and Graham Williams (SIAT, CAS)

}

\keyword{ models }
\keyword{ classif }
