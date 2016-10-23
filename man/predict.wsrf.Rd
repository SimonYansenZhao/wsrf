\name{predict.wsrf}

\alias{predict}
\alias{predict.wsrf}

\title{ Predict Method for \code{wsrf} Model }

\description{ Give the predictions for the new data by the forest
  model built from \code{wsrf}.  }

\usage{ \method{predict}{wsrf}(object, newdata, type=c("response",
  "class", "vote", "prob", "aprob", "waprob"), \dots) }

\arguments{

  \item{object}{object of class \code{wsrf}.}

  \item{newdata}{the data that needs to be predicted.  Its format
    should be the same as that for \code{\link{wsrf}}.}

  \item{type}{the type of prediction required, a character vector indicating
    the types of output, and can be one of the values below:
    \describe{
      \item{vote}{matrix of vote counts}
      \item{response}{predicted values.}
      \item{class}{the same as response.}
      \item{prob}{matrix of class probabilities.  The probability is the
        proportion of trees in the forest voting for the particular
        outcome (prob = votes / ntree)}

      \item{aprob}{the average score from the decision trees for each
        class rather than the proportion of decision trees for each
        class (aprob = scores / ntree)}
	  
      \item{waprob}{the weighted average, weighted by the accuracy of
        the tree (waprob = scores * accuracy / sum(accuracy))}
    }}
      
  \item{\dots}{optional additional arguments. At present no additional
    arguments are used.}

}

\value{a list of predictions for the new data with corresponding components for 
  each type of predictions.  For \code{type=class} or \code{type=class}, a 
  vector of length \code{nrow(newdata)}, otherwise, a matrix of
  \code{nrow(newdata) * (number of class label)}.  For example, if given 
  \code{type=c("class", "prob")} and the return value is \code{res}, then 
  \code{res$class} is a vector of predicted class labels of length 
  \code{nrow(newdata)}, and \code{res$prob} is a matrix of class probabilities.}

\author{
  He Zhao and Graham Williams (SIAT)
}

\seealso{
  \code{\link{wsrf}}
}
