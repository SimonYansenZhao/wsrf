\name{strength.wsrf}

\alias{strength}
\alias{strength.wsrf}

\title{
  Strength
}

\description{ Give the measure for the collective performance of
  individual trees in the forest model built from \code{wsrf}.  }

\usage{
\method{strength}{wsrf}(object, \dots)
}

\arguments{

  \item{object}{object of class \code{wsrf}.}

  \item{\dots}{optional additional arguments. At present no additional
    arguments are used.}

}

\details{
  The measure was introduced in Breiman (2001).
}

\value{
  A numeric value.
}

\references{

  Breiman, L.  2001 "Random forests".  \emph{Machine learning},
  \bold{45(1)}, 5--32.

}

\author{
  He Zhao and Graham Williams (SIAT, CAS)
}

\seealso{
  \code{\link{wsrf}}
}
