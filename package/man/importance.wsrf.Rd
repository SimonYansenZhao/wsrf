\name{importance.wsrf}
\alias{importance}
\alias{importance.wsrf}
\title{Extract Variable Importance Measure}
\description{
  This is the extractor function for variable importance measures as
  produced by \code{wsrf}.
}
\usage{
\method{importance}{wsrf}(x, type=NULL, class=NULL, scale=TRUE, ...)
}
\arguments{
  \item{x}{an object of class \code{wsrf}.}
  \item{type}{either 1 or 2, specifying the type of importance measure
    (1=mean decrease in accuracy, 2=mean decrease in node impurity).}
  \item{class}{for classification problem, which class-specific measure
    to return.}
  \item{scale}{For permutation based measures, should the measures be
    divided their ``standard errors''?}
  \item{...}{not used.}
}
\value{
  A matrix of importance measure, one row for each predictor variable.
  The column(s) are different importance measures.
}
\details{
  Here are the definitions of the variable importance measures.  The
  first measure is computed from permuting OOB data:  For
  each tree, the prediction error on the out-of-bag portion of the
  data is recorded.
  Then the same is done after permuting each predictor variable.  The
  difference between the two are then averaged over all trees, and
  normalized by the standard deviation of the differences.

  The second measure is the total decrease in node impurities from
  splitting on the variable, averaged over all trees.  The node 
  impurity is measured by the Information Gain Ratio index.
}
%\references{
%}
\seealso{
  \code{randomForest}
}

%\author{}
\keyword{classif}
