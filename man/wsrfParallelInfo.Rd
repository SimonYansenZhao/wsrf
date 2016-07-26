\name{wsrfParallelInfo}

\alias{parallel}
\alias{wsrfParallelInfo}

\title{
  Query about underlying parallel implementation information
}

\description{ Give the information about underlying parallel
  implementation. }

\usage{
wsrfParallelInfo(...)
}

\arguments{
  
  \item{\dots}{Optional additional arguments. At present no additional
  arguments are used.}

}


\details{ The parallel implementation cannot be changed after
  installation of the package.  So this function is used to query which
  techniques are actually adopted as the underlying parallel
  implementation, among which are C++11, Boost, or no parallelism at
  all.  }

\value{ A diagnostic message about the parallel implementation depends
  on the actual situation.  Currently possible messages are:
  \enumerate{
    \item With parallel computing disabled
    \item Use C++ standard thread library for parallel computing
    \item Use C++ Boost thread library for parallel computing
  }
}

\author{
  He Zhao (SIAT)
}

\seealso{
  \code{\link{wsrf}}
}
