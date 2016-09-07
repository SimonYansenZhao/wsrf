\name{print.wsrf}

\alias{print}
\alias{print.wsrf}

\title{ Print Method for \code{wsrf} model }

\description{

  Print a summary of the forest model or one specific tree in the forest
  model built from \code{wsrf}.

}

\usage{
\method{print}{wsrf}(x, trees, \dots)
}

\arguments{

  \item{x}{object of class \code{wsrf}.}

  \item{trees}{the index of a specific tree.  If missing, \code{print}
    will print a summary of the model.}

  \item{\dots}{optional additional arguments. At present no additional
    arguments are used.}

}

\author{
  He Zhao and Graham Williams (SIAT)
}

\seealso{
  \code{\link{wsrf}}
}
