\name{combine.wsrf}
\alias{combine}
\alias{combine.wsrf}

\title{Combine Ensembles of Trees}

\description{
Combine two more more ensembles of trees into one.
}

\usage{
combine(...)
}

\arguments{

  \item{...}{two or more objects of class \code{randomForest}, to be
    combined into one.}

}

\value{
An object of class \code{wsrf}.
}

\examples{
  library("wsrf")

  # Prepare parameters.
  ds     <- rattle::weather
  target <- "RainTomorrow"
  vars   <- setdiff(names(ds), c("Date", "Location", "RISK_MM"))
  if (sum(is.na(ds[vars]))) ds[vars] <- randomForest::na.roughfix(ds[vars])
  ds[target] <- as.factor(ds[[target]])
  form <- as.formula(paste(target, "~ ."))
  set.seed(42)
  train.1 <- sample(nrow(ds), 0.7*nrow(ds))
  test.1  <- setdiff(seq_len(nrow(ds)), train.1)

  set.seed(49)
  train.2 <- sample(nrow(ds), 0.7*nrow(ds))
  test.2  <- setdiff(seq_len(nrow(ds)), train.2)
  
  # Build model.  We disable parallelism here, since CRAN Repository
  # Policy (https://cran.r-project.org/web/packages/policies.html)
  # limits the usage of multiple cores to save the limited resource of
  # the check farm.

  model.wsrf.1 <- wsrf(form, data=ds[train.1, vars], parallel=FALSE)
  model.wsrf.2 <- wsrf(form, data=ds[train.2, vars], parallel=FALSE)

  
  # Merge two models.
  model.wsrf.big <- combine.wsrf(model.wsrf.1, model.wsrf.2)
  print(model.wsrf.big)
  cl <- predict(model.wsrf.big, newdata=ds[test.1, vars], type="response")
  actual <- ds[test.1, target]
  (accuracy.wsrf <- sum(cl==actual)/length(actual))

}


\seealso{\code{\link{subset}}}