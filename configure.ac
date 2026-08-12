AC_PREREQ([2.69])
AC_INIT([audit_hook], [1.0.0], [bugs@example.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_PROG_CC

# Require C++20
CXXFLAGS="$CXXFLAGS -std=c++20"

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
])
AC_OUTPUT

