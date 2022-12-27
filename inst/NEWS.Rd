\name{NEWS}
\title{News for Package 'wsrf'}

\newcommand{\pkglink}{\href{https://CRAN.R-project.org/package=#1}{\pkg{#1}}}

\section{Changes in wsrf version 1.7.30 (2022-12-27)}{
  \subsection{Bug Fixes}{
    \itemize{

      \item Replace \code{sprintf} with \code{snprintf}.

    }
  }
}

\section{Changes in wsrf version 1.7.27 (2022-03-13)}{
  \subsection{CRAN Checks}{
    \itemize{

      \item Drop the dependency on \pkglink{rattle.data} since it's not
      available.

    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Use the \code{iris} dataset in examples.

      \item Remove dependencies on \pkglink{party} and
      \pkglink{markdown}.

      \item Correct typos.

    }
  }

}

\section{Changes in wsrf version 1.7.22 (2021-10-10)}{
  \subsection{Documentation}{
    \itemize{

      \item Replace JSS URLs with DOIs.

    }
  }

}


\section{Changes in wsrf version 1.7.21 (2021-04-28)}{
  \subsection{Documentation}{
    \itemize{

      \item Correct invalid links in NEWS.

    }
  }

}


\section{Changes in wsrf version 1.7.20 (2021-04-24)}{
  \subsection{Bug Fixes}{
    \itemize{

      \item Correct the uninitialized variable \code{pos} in
      src/c4_5_var_selector.cpp.

    }
  }

  \subsection{CRAN Checks}{
    \itemize{

      \item Suggest \pkglink{markdown} and \pkglink{rmarkdown} as
      dependency (See
      \href{https://github.com/yihui/knitr/issues/1864}{Move the
      markdown package from Imports to Suggests #1864}).

    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Add authors' ORCID identifiers.

      \item Change http to https.

    }
  }

}

\section{Changes in wsrf version 1.7.17 (2017-09-25)}{
  \subsection{Bug Fixes}{
    \itemize{

      \item Use \pkglink{rattle.data} for datasets instead of
      \pkglink{rattle}.

    }
  }

}

\section{Changes in wsrf version 1.7.13 (2017-04-17)}{
  \subsection{CRAN Checks}{
    \itemize{

      \item Remove deprecated Make variables with prefix \code{CXX1X}
      (\href{https://developer.r-project.org/blosxom.cgi/R-devel/NEWS/2017/04/01#n2017-04-01}{CHANGES
      IN R 3.4.0}).

    }
  }

}

\section{Changes in wsrf version 1.7.10 (2017-03-28)}{
  \subsection{Enhancements}{
    \itemize{

      \item Increase the responsiveness when user interrupt.

      \item Add registration for native routines.

    }
  }

  \subsection{Bug Fixes}{
    \itemize{

      \item Segment fault and memory not mapped when user interrupt.

    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Add description for installation from GitHub.

      \item Add citation info.

    }
  }

}

\section{Changes in wsrf version 1.7.0 (2016-10-27)}{
  \subsection{Changes}{
    \itemize{

      \item Change the output of \code{predict.wsrf} to be a list of
      predictions of different types.

    }
  }

  \subsection{Bug Fixes}{
    \itemize{

      \item Fix bug introduced in version 1.6.25 when adding
      \code{wsrf.default} for separate inputs and response.

    }
  }

  \subsection{Enhancements}{
    \itemize{

      \item Throw more informative error when variable (attribute) not
      found in \code{newdata} of \code{predict.wsrf}.

      \item Add interrupt check in \code{predict.wsrf}.

    }
  }
}

\section{Changes in wsrf version 1.6.25 (2016-09-10)}{
  \subsection{New Features}{
    \itemize{

      \item Add a new argument \code{nodesize} to change the minimum
      node size.

      \item Add a method \code{wsrf.default} to deal with separate
      inputs (\code{x}) and response (\code{y}), besides
      \code{wsrf.formula} with \code{formula}.

    }
  }

  \subsection{Changes}{
    \itemize{

      \item Drop support of Boost for multithreading.

    }
  }

  \subsection{Enhancements}{
    \itemize{

      \item Speed up data loading in \code{wsrf} and \code{predict}.

    }
  }

  \subsection{Misc}{
    \itemize{

      \item Add subject classifications for the content of the package.

      \item Add badges in README.

    }
  }

}

\section{Changes in wsrf version 1.5.46 (2016-07-11)}{

  \subsection{Bug Fixes}{
    \itemize{

      \item Fix undefined column error when column name is a
      non-syntactic name, such as names contain whitespace.

      \item Fix failure to switch to the code for the right compiler
      version.

    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Add URL and BugReports into DESCRIPTION.

    }
  }

}

\section{Changes in wsrf version 1.5.29 (2015-10-10)}{

  \subsection{Bug Fixes}{
    \itemize{

      \item Fix out-of-bound access bug triggered by passing test data
      without the column of target variable.

    }
  }

}

\section{Changes in wsrf version 1.5.24 (2015-07-07)}{

  \subsection{Bug Fixes}{
    \itemize{

      \item Add missing imports to correct the NOTEs from
      \href{https://developer.r-project.org/blosxom.cgi/R-devel/NEWS/2015/06/29#n2015-06-29}{the
      newly added check item of R-devel on 2015-06-29}.

      \item Correct the URLs of CRAN package pages to their canonical
      form in the documentation.

    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Add more examples in the manual.

      \item Update documentation.

      \item Change vignette from Rnw to Rmd.

    }
  }

}

\section{Changes in wsrf version 1.5.14 (2015-06-09)}{

  \subsection{Bug Fixes}{
    \itemize{

      \item Negative information gain when splitting nodes leads to
      \code{-nan} when normalized by \code{sqrt()}.

      \item No-matched predicted results when predict type is
      "\code{prob}" (Thank \emph{Erich Studerus} for reporting this
      bug).

      \item Fix a bug when assessing variable importance: Vector out of
      range access.

    }
  }

  \subsection{Misc}{
    \itemize{

      \item Update documentation: Make it clear that the internal
      algorithm used.

      \item Move all predict functionality from \code{R} into \code{C++}
      code.

      \item Correct all compilation warnings on Windows, Linux, and OS X
      and errors from memtest.

    }
  }

}

\section{Changes in wsrf version 1.5.0 (2015-05-24)}{

  \subsection{New Features}{
    \itemize{

      \item Add new function \code{subset} to get a sub-forest from a
      \pkglink{wsrf} model.
      
      \item Add new function \code{combine} to merge multiple
      \pkglink{wsrf} models into bigger one.

      \item Add \code{confusion}, \code{oot.times}, \code{predicted},
      \code{useweights}, \code{mtry} in the \pkglink{wsrf} model list.

      \item Add \code{clusterlogfile} in the parameter list of function
      \code{wsrf} for debug.

    }
  }

  \subsection{Changes}{
    \itemize{

      \item Function \code{predict.wsrf} returns NAs when there are
      missing values in \code{newdata}.

      \item Remove function \code{summary.wsrf}, moving its
      functionality into \code{print.wsrf}, so that the default outputs
      of \code{print.wsrf} gives a summary of the model.

      \item Tree structure printed by function \code{print.wsrf} is
      slightly different from previous version: Decisions from the same
      test node (internal node) have the same item number.

      \item Function \code{oobErrorRate} is renamed into
      \code{oob.error.rate} and made as generic.

    }
  }

  \subsection{Enhancements}{
    \itemize{

      \item Refactor code, making rooms for future modifications: Remove
      unused code; Redefine some classes.

      \item Operate on R objects directly in C++ code instead of copied
      ones.

      \item Improve time performance.

    }
  }

  \subsection{Bug Fixes}{
    \itemize{

      \item Correct weighted random sampling.

    }
  }

  \subsection{Misc}{
    \itemize{

      \item Replace \code{srand()} with \code{R} \emph{RNG} functions.

    }
  }

}

\section{Changes in wsrf version 1.4.0 (2014-05-28)}{


  \subsection{New Features}{
    \itemize{

      \item Add new function \code{wsrfParallelInfo} for querying the
      underlying parallel implementation (parallel or not).

    }
  }

  \subsection{Bug Fixes}{
    \itemize{

      \item The predict method requires a dummy outcome column to be in
      the new data (Thank \emph{Max Kuhn} for reporting this bug).

    }
  }

  \subsection{Minor Changes}{
    \itemize{

      \item Adapt R's make variable \env{CXX_STD} to configure
      installation.

      \item List packages used by vignettes code in DESCRIPTION.
    }
  }

  \subsection{Documentation}{
    \itemize{

      \item Let R to build the vignettes so that it can be visible on
      \pkglink{wsrf}.

    }
  }

}


\section{Changes in wsrf version 1.3.0 (2014-02-08)}{
  \itemize{

    \item added documents:
    \itemize{
      \item vignettes
      \item README.Rd
      \item NEWS.md
    }

    \item provided Windows support (no multi-threading functionality,
    but can run in distributed way)

    \item bug fixed: when used in distributed environment, variable
    name info is missing, therefore, "Error: Training Set is empty"
    occurs.

  }
}
