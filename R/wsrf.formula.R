wsrf.formula <- function(formula, data, ...)
{
  # Determine the information provided by the formula.

  target <- as.character(formula[[2]]) # Assumes it is a two sided formula.
  target <- sub("^`(.*)`$", "\\1", target)  # Remove backticks
  target.ind <- which(colnames(data) == target)
  if (length(target.ind) == 0) stop("The named target must be included in the dataset.")
  if (formula[[3]] == ".") {
    inputs <- (1:ncol(data))[-target.ind]
    vars   <- union(inputs, target.ind)
  } else {
    inputs <- attr(terms.formula(formula, data=data), "term.labels")
    inputs <- sub("^`(.*)`$", "\\1", inputs)  # Remove backticks if variable names are non-syntactic
    vars   <- union(inputs, target)
  }

  # Retain just the dataset required.

  data <- as.data.frame(data[vars])
  
  # Split data into inputs x and response y.

  x <- data[-length(data)]
  y <- data[[length(data)]]
#  rm(data)

  model <- wsrf.default(x, y, ...)
  return(model)
}

