\name{oob.error.rate.wsrf}

\alias{oob.error.rate}
\alias{oob.error.rate.wsrf}

\concept{out-of-bag error rate}

\title{
  Out-of-Bag Error Rate
}

\description{
  Return out-of-bag error rate for "\code{wsrf}" model.
}

\usage{
\method{oob.error.rate}{wsrf}(object, tree, ...)
}

\arguments{
  \item{object}{object of class \code{wsrf}.}
  \item{tree}{logical or an integer vector for the index of a specific
      tree in the forest model.  If provided as an integer vector,
      \code{oobErrorRate.wsrf} will give the corresponding out-of-bag
      error rates of the exact trees specified by \code{tree}.  If
      TRUE, all error rates will be presented.  If FALSE or missing,
      the gross error rate for the forest will be given.}
  \item{...}{not used.}
}

\value{
  return a vector of error rates.
}

\author{
  He Zhao and Graham Williams (SIAT)
}

\seealso{
  \code{\link{wsrf}}
}
