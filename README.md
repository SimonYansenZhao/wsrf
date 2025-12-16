# wsrf: An R Package for Scalable Weighted Subspace Random Forests

[![License](https://img.shields.io/badge/license-GPL%20%28%3E%3D%203%29-brightgreen.svg?style=flat)](https://www.gnu.org/licenses/gpl-3.0.html)
[![Version on CRAN](http://www.r-pkg.org/badges/version/wsrf)](https://cran.r-project.org/package=wsrf)
[![Number of downloads from RStudio CRAN mirror](https://cranlogs.r-pkg.org/badges/wsrf)](https://www.r-pkg.org/pkg/wsrf)

The [wsrf](https://cran.r-project.org/package=wsrf) is a parallel
implementation of the Weighted Subspace Random Forest algorithm (wsrf)
of [Xu et al](https://dx.doi.org/10.4018/jdwm.2012040103).  A novel
variable weighting method is used for variable subspace selection in
place of the traditional approach of random variable sampling.  This
new approach is particularly useful in building models for high
dimensional data---often consisting of thousands of variables.
Parallel computation is used to take advantage of multi-core machines
and clusters of machines to build random forest models from high
dimensional data with reduced elapsed times.

## Documentation & Examples

The package ships with a [html
vignette](https://cran.r-project.org/package=wsrf/vignettes/wsrf-guide.html)
including more details and a few examples.


## Installation

Currently, wsrf requires [R](https://www.r-project.org/) (>= 3.3.0),
[Rcpp](https://cran.r-project.org/package=Rcpp) (>= 0.10.2).  For the
use of multi-threading, a C++ compiler with
[C++11](https://en.wikipedia.org/wiki/C%2B%2B11) standard support of
threads (for example, [GCC](https://gcc.gnu.org/) 4.8.1) is required.
Since the latest version of R has added support for C++11 on all
operating systems, we do not provide support for the old version of R
and C++ compiler without C++11 support.  To install the latest version
of the package, from within R run:

```R
R> install.packages("wsrf")
```

## NOTE

Previous version of wsrf provide support on systems without C++11 or
using Boost for multithreading.  Though we do not provide support for
these options anymore, but still leave the usage here for someone with
needs of previous version of wsrf.  The choice is available at
installation time depending on what is available to the user:

```R
# To install previous version of wsrf without C++11
R> install.packages("wsrf", type = "source", configure.args = "--enable-c11=no")

# To install previous version of wsrf with Boost for multithreading
R> install.packages("wsrf",
+                   type = "source",
+                   configure.args = "--with-boost-include=<Boost include path>
                                      --with-boost-lib=<Boost lib path>")
```

After installation, one can use the built-in function
`wsrfParallelInfo` to query whether the version installed is what they
really want (distributed or multi-threaded).

```R
R> wsrfParallelInfo()
```


## License

GPL (>= 3)
