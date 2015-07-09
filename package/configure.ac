AC_INIT(wsrf, 1.5.0)

AC_ARG_WITH(boost-include, [  --with-boost-include=PATH set Boost include path])
AC_ARG_WITH(boost-lib, [  --with-boost-lib=PATH set Boost lib path])

AC_ARG_ENABLE(c11, [  --enable-c11  use C++11 for parallel computing])

if test [ -n "$with_boost_include" ] -a [ -n "$with_boost_lib" ]
then
  AC_MSG_NOTICE([Use Boost for parallel computing])
  PKG_CPPFLAGS="-DWSRF_USE_BOOST"
  PKG_CXXFLAGS="-I$with_boost_include"
  PKG_LIBS="-L$with_boost_lib -lboost_thread -lboost_system -lboost_chrono"
  CXX_STD=
  VERSION_INFO="Use C++ Boost thread library for parallel computing"

elif test "$enable_c11" == "yes"
then
  AC_MSG_NOTICE([Use C++11 for parallel computing])
  PKG_CPPFLAGS="-DWSRF_USE_C11"
  PKG_CXXFLAGS=
  PKG_LIBS="-pthread"
  CXX_STD="CXX11"
  VERSION_INFO="Use C++ standard thread library for parallel computing"

else
  AC_MSG_NOTICE([With parallel computing disabled])
  PKG_CPPFLAGS=
  PKG_CXXFLAGS=
  PKG_LIBS=
  CXX_STD=
  VERSION_INFO="With parallel computing disabled"

fi

AC_SUBST(PKG_CPPFLAGS)
AC_SUBST(PKG_CXXFLAGS)
AC_SUBST(PKG_LIBS)
AC_SUBST(CXX_STD)
AC_SUBST(VERSION_INFO)
dnl AC_CONFIG_FILES([src/Makevars src/Makevars.win])
AC_CONFIG_FILES([src/Makevars])
AC_CONFIG_FILES([R/wsrf.R])
AC_CONFIG_FILES([R/wsrfParallelInfo.R])
AC_OUTPUT
