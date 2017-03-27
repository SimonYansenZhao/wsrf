predict.wsrf <- function(object,
                         newdata,
                         type=c("response",
                                "class",
                                "vote",
                                "prob",
                                "aprob",
                                "waprob"),
                         ...)
{
  if (!inherits(object, "wsrf"))
    stop("Not a legitimate wsrf object")

  # "class" is the default type.

  if (missing(type)) type <- "class"

  # Several types are allowed.

  type <- match.arg(type, several.ok = TRUE)

  # type "reponse" is the same as "class"

  hasResponseType <- ifelse("response" %in% type, TRUE, FALSE)
  hasClassType <- ifelse("class" %in% type, TRUE, FALSE)

  if (hasClassType && hasResponseType) {
    type <- type[-which(type == "response")]
  } else if (!hasClassType && hasResponseType) {
    type[which(type == "response")] <- "class"
  }

  # Convert string type into integer flag.

  type <- sum(sapply(type, function(x) {
    switch(x, class=1, vote=2, prob=4, aprob=8, waprob=16)
  }))

  # The C++ code for prediction does not handle missing values.  So handle
  # them here by removing them from the dataset and then add in, in
  # the correct places, NA as the results from predict.

  complete <- complete.cases(newdata)
  rnames   <- rownames(newdata)
  newdata  <- newdata[complete,]

  hasmissing <- !all(complete)
  nobs       <- length(complete)

  res <- .Call(WSRF_predict, object, newdata, type)
  names(res) <- c("class", "vote", "prob", "aprob", "waprob")
  

  # Deal with names and observations with missing values.

  res <- sapply(names(res), function(ty) {
    pred <- res[[ty]]

    if (is.null(pred)) return(pred)

    if (ty == "class") {
      if (hasmissing) {
        temp <- factor(rep(NA, nobs), levels=levels(pred))
        temp[complete] <- pred
        pred <- temp
      }
      names(pred) <- rnames
      return(pred)
    } else {
      if (hasmissing) {
        temp <- matrix(NA_real_, nrow=nobs, ncol=ncol(pred))
        temp[complete, ] <- pred
        colnames(temp) <- colnames(pred)
        pred <- temp
      }
      rownames(pred) <- rnames
      return(pred)
    }
  }, simplify=FALSE)

  # In case users aren't aware that type "reponse" is the same as "class".

  if (hasResponseType) res[["response"]] <- res[["class"]]

  return(res)
}
