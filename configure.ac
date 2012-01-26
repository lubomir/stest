AC_PREREQ([2.68])
AC_INIT([stest], [0.1], [lubomir.sedlar@gmail.com])
AC_CONFIG_SRCDIR([src/stest.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

AC_ARG_ENABLE([tests],
              AS_HELP_STRING([--disable-tests], [disable building tests]))

AS_IF([test "x$enable_tests" != "xno"], [
      AC_CHECK_CUTTER
      ])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" != "xno"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT([Makefile])
