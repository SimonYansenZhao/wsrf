\name{subset.wsrf}
\alias{subset}
\alias{subset.wsrf}

\title{Subset of a Forest}

\description{
Obtain a subset of a forest.
}

\usage{
\method{subset}{wsrf}(x, trees, ...)
}

\arguments{

  \item{x}{an object of class \code{wsrf}.}

  \item{trees}{which trees should be included in the sub-forest.  An
    integer vector, which indicates the index of the trees.}

  \item{...}{not used.}

}

\value{
An object of class \code{wsrf}.
}

\examples{
  library("wsrf")

  # Prepare parameters.
  ds     <- iris
  target <- "Species"
  vars   <- names(ds)
  if (sum(is.na(ds[vars]))) ds[vars] <- randomForest::na.roughfix(ds[vars])
  ds[target] <- as.factor(ds[[target]])
  form <- as.formula(paste(target, "~ ."))
  set.seed(42)
  train <- sample(nrow(ds), 0.7*nrow(ds))
  test  <- setdiff(seq_len(nrow(ds)), train)
  
  # Build model.  We disable parallelism here, since CRAN Repository
  # Policy (https://cran.r-project.org/web/packages/policies.html)
  # limits the usage of multiple cores to save the limited resource of
  # the check farm.

  model.wsrf <- wsrf(form, data=ds[train, vars], parallel=FALSE)
  print(model.wsrf)
  
  # Subset.
  submodel.wsrf <- subset.wsrf(model.wsrf, 1:200)
  print(submodel.wsrf)
  cl <- predict(submodel.wsrf, newdata=ds[test, vars], type="response")$response
  actual <- ds[test, target]
  (accuracy.wsrf <- mean(cl==actual))
}


\seealso{\code{\link{combine}}}