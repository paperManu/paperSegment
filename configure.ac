#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT([PAPERSEGMENT],
        [0.0.1],
        [https://bitbucket.org/MogwaiFearSatan/papersegment],
        [papersegment],
        [https://bitbucket.org/MogwaiFearSatan/papersegment])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE()

# Check for C and C++ compilers
AC_PROG_CXX
AC_PROG_CC

# Check for header files
AC_HEADER_STDC

# OpenCV
PKG_CHECK_MODULES([OPENCV], [opencv >= 2.3])
if test "x${have_opencv}" = "xfalse" ; then
    AC_MSG_ERROR([Missing opencv])
fi

# Freenect
PKG_CHECK_MODULES([FREENCT], [libfreenect >= 0.1])
if test "x${have_freenect}" = "xfalse" ; then
    AC_MSG_ERROR([Missing freenect])
fi

# Boost
BOOST_REQUIRE([1.35])
BOOST_THREADS

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
    AM_MISSING_PROG(HELP2MAN, help2man)
else
    HELP2MAN=:
fi

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    man/Makefile
    ])
AC_OUTPUT

