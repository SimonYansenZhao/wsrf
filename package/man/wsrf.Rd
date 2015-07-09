\name{wsrf}

\alias{wsrf}

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

wsrf(formula, data, nvars, mtry, ntrees=500, weights=TRUE, 
                 parallel=TRUE, na.action=na.fail, importance=FALSE, clusterlogfile)

}

\arguments{
  
  \item{formula}{a formula, with a response but no interaction terms.}

  \item{data}{a data frame in which to interpret the variables named in
          the formula.}

  \item{ntrees}{number of trees to build on each server; By default,
  500}

  \item{nvars, mtry}{number of variables to choose, by default, being
      the integer less than or equal to \eqn{log_2(ninputs) + 1}.  For
      compatibility with other \code{R} packages like
      \code{randomForest}, both \code{nvars} and \code{mtry} are
      supported, however, only one of them should be specified.}

  \item{weights}{logical.  \code{TRUE} for weighted subspace selection,
      which is the default; \code{FALSE} for random selection, and the
      trees are based on C4.5.}

  \item{na.action}{indicate the behaviour when encountering NA values in
      \code{data}.}

  \item{parallel}{whether to run multiple cores (TRUE), nodes, or
      sequentially (FALSE).}

  \item{importance}{should importance of predictors be assessed? }

  \item{clusterlogfile}{character.  The pathname of the log file when
      building model in a cluster.  For debug.}

}

\details{

  See Xu, Huang, Williams, Wang, and Ye (2012) for more details of
  the algorithm.

  Currently, \pkg{wsrf} can only be used for classification.  When
  \code{weights=FALSE}, C4.5-based trees (Quinlan (1993)) are grown by
  \code{wsrf}, where binary split is used for continuous predictors
  (variables) and \emph{k}-way split for categorical ones.  For
  continuous predictors, each of the values themselves is used as split
  points, no discretization used.  The only stopping condition for split
  is the minimum node size is 2.

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
  ds <- rattle::weather
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
  
  # Build model.
  model.wsrf <- wsrf(form, data=ds[train, vars])
  
  # View model.
  print(model.wsrf)
  print(model.wsrf, tree=1)
  
  # Evaluate.
  strength(model.wsrf)
  correlation(model.wsrf)
  cl <- predict(model.wsrf, newdata=ds[test, vars], type="response")
  actual <- ds[test, target]
  (accuracy.wsrf <- sum(cl==actual)/length(actual))
}

\references{
  
  Xu B, Huang JZ, Williams G, Wang Q, Ye YM
  (2012). "Classifying very high-dimensional data with random forests built from small subspaces."
  \emph{International Journal of Data Warehousing and Mining (IJDWM)},
  8(2), 44-63.

  Quinlan
  J. R. (1993). "C4.5: Programs for Machine Learning". \emph{Morgan
  Kaufmann}.
  
}

\author{
  
  He Zhao and Graham Williams (SIAT)
  
}

\keyword{ models }
\keyword{ classif }
