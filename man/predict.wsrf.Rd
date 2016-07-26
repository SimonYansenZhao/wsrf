\name{predict.wsrf}

\alias{predict}
\alias{predict.wsrf}

\title{ Predict Method for \code{wsrf} Model }

\description{ Give the predictions for the new data by the forest
  model built from \code{wsrf}.  }

\usage{ \method{predict}{wsrf}(object, newdata, type=c("response",
  "class", "prob", "vote", "aprob", "waprob"), \dots) }

\arguments{
  \item{object}{object of class \code{wsrf}.}
  \item{newdata}{the data that needs to be predicted.  Its format
      should be the same as that for \code{\link{wsrf}}.}
  \item{type}{ the type of prediction required, indicating the type of
      output, and can be one of the values below:
      \describe{
        \item{vote}{matrix of vote counts}
        \item{response}{predicted values.}
        \item{class}{the same as response.}
        \item{prob}{matrix of class probabilities.  The probability is
            the proportion of trees in the forest voting for the
            particular outcome (prob = votes / ntrees)}
        \item{aprob}{the average score from the decision trees for
            each class rather than the proportion of decision trees
            for each class (aprob = scores / ntrees)}
        \item{waprob}{the weighted average, weighted by the accuracy
            of the tree (waprob = scores * accuracy / sum(accuracy))}
        }}
  \item{\dots}{Optional additional arguments. At present no additional
  arguments are used.}
}

\value{ a vector of length \code{nrow(newdata)} if given \code{type} of
    \code{response} or \code{class}, otherwise, a matrix of
  \code{nrow(newdata) * (number of class label)}.}

\author{
  He Zhao and Graham Williams (SIAT)
}

\seealso{
  \code{\link{wsrf}}
}
