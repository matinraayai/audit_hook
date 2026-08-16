AC_PREREQ([2.69])
AC_INIT([audit_hook], [1.0.0], [bugs@example.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_PROG_CC

# Require C++20
CXXFLAGS="$CXXFLAGS -std=c++20"

# Check for Doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
    AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

# Create an Automake conditional so Makefiles know if Doxygen is available
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

# Check for jemalloc to determine if we can build the jemalloc_injector example
AC_CHECK_LIB([jemalloc], [je_malloc], [HAVE_JEMALLOC=yes], [HAVE_JEMALLOC=no])
AM_CONDITIONAL([BUILD_JEMALLOC_EXAMPLE], [test "$HAVE_JEMALLOC" = "yes"])

# Tell Autoconf to process Doxyfile.in -> Doxyfile
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
    examples/Makefile
    examples/jemalloc_injector/Makefile
    Doxyfile
])

AC_OUTPUT

