# wsrf: An R Package for Scalable Weighted Subspace Random Forests

The [wsrf](http://cran.r-project.org/package=wsrf) is a parallel
implementation of the Weighted Subspace Random Forest algorithm (wsrf)
of \cite{xu2012classifying}.  A novel variable weighting method is
used for variable subspace selection in place of the traditional
approach of random variable sampling.  This new approach is
particularly useful in building models for high dimensional
data---often consisting of thousands of variables.  Parallel
computation is used to take advantage of multi-core machines and
clusters of machines to build random forest models from high
dimensional data with reduced elapsed times.

## Documentation & Examples

The package ships with a PDF vignette including more details and a few
examples.


## Installation

Currently, wsrf requires R (>= 3.0.0),
[Rcpp](http://cran.r-project.org/package=Rcpp) (>= 0.10.2).  For the
use of multi-threading, a C++ compiler with
[C++11](http://en.wikipedia.org/wiki/C%2B%2B11) standard support of
threads or [the Boost C++ library](http://www.boost.org/) with version
above 1.54 is required. The choice is available at installation time
depending on what is available to the user.  To install the latest
version of the package, from within \proglang{R} run:

```R
install.packages("wsrf", type = "source", configure.args = "--enable-c11=yes")
```

The latest [GCC](https://gcc.gnu.org/projects/cxx0x.html) supports
C++11, and if C++11 is not available or the Operating System you want
to run wsrf is Windows:

```R
install.packages("wsrf", type = "source", configure.args = "--enable-c11=no")
```

or just:

```R
install.packages("wsrf")
```

Finally if you want to use Boost for multithreading, wsrf can be
installed with the appropriate configuration options:

```R
install.packages("wsrf",
                 type = "source",
                 configure.args = "--with-boost-include=<Boost include path>
                                   --with-boost-lib=<Boost lib path>")
```

A useful function `wsrfParallelInfo()` can be used for checking
parallelism is used or not.

## License

GPL (>= 2)
