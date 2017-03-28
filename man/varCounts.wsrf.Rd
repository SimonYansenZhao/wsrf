\name{varCounts.wsrf}

\alias{varCounts.wsrf}

\title{
  Number of Times of Variables Selected as Split Condition
}

\description{
  Return the times of each variable being selected as split condition.
  For evaluating the bias of wsrf towards attribute types (categorical and
  numerical) and the number of values each attribute has.
}

\usage{
\method{varCounts}{wsrf}(object)
}

\arguments{
  \item{object}{object of class \code{wsrf}.}
}

\value{ A vector of integer.  The length is the same as the training
  data for building that \code{wsrf} model. }

\author{
  He Zhao and Graham Williams (SIAT, CAS)
}

\seealso{
  \code{\link{wsrf}}
}
