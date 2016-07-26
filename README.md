# wsrf: An R Package for Scalable Weighted Subspace Random Forests

The [wsrf](http://cran.r-project.org/package=wsrf) is a parallel
implementation of the Weighted Subspace Random Forest algorithm (wsrf)
of [Xu et al](http://dx.doi.org/10.4018/jdwm.2012040103).  A novel variable weighting method is
used for variable subspace selection in place of the traditional
approach of random variable sampling.  This new approach is
particularly useful in building models for high dimensional
data---often consisting of thousands of variables.  Parallel
computation is used to take advantage of multi-core machines and
clusters of machines to build random forest models from high
dimensional data with reduced elapsed times.

## Documentation & Examples

The package ships with a
[html vignette](http://cran.r-project.org/web/packages/wsrf/vignettes/wsrf-guide.html)
including more details and a few examples.


## Installation

Currently, wsrf requires [R](http://www.r-project.org/) (>= 3.3.0),
[Rcpp](http://cran.r-project.org/package=Rcpp) (>= 0.10.2).  For the
use of multi-threading, a C++ compiler with
[C++11](http://en.wikipedia.org/wiki/C%2B%2B11) standard support of
threads (for example,
[GCC](http://gcc.gnu.org/) 4.8.1) is required. The choice is available at installation time
depending on what is available to the user.  To install the latest
version of the package, from within R run:

```R
R> install.packages("wsrf", type = "source", configure.args = "--enable-c11=yes")
```

The latest [GCC](https://gcc.gnu.org/projects/cxx0x.html) supports
C++11, and if C++11 is not available or the Operating System you want
to run wsrf is Windows:

```R
R> install.packages("wsrf", type = "source", configure.args = "--enable-c11=no")
```

or just:

```R
R> install.packages("wsrf")
```

Finally if you want to use Boost for multithreading, wsrf can be
installed with the appropriate configuration options:

```R
R> install.packages("wsrf",
+                   type = "source",
+                   configure.args = "--with-boost-include=<Boost include path>
                                      --with-boost-lib=<Boost lib path>")
```

## After Installation

After installation, one can use the built-in function
`wsrfParallelInfo` to query whether the version installed is what they
really want (distributed or multi-threaded).

```R
R> wsrfParallelInfo()
```


## License

GPL (>= 2)
