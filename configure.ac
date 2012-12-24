AC_PREREQ([2.68])
AC_INIT([stest], [0.1], [lubomir.sedlar@gmail.com])
AC_CONFIG_SRCDIR([src/stest.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AM_SILENT_RULES

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

AC_ARG_ENABLE([tests],
              AS_HELP_STRING([--disable-tests], [disable building tests]))

AS_IF([test "x$enable_tests" != "xno"], [
      AC_CHECK_CUTTER
      ])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" != "xno"])

AC_CONFIG_FILES([doc/stest.1 doc/gen-test.1])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Does compiler support -Wall
AC_MSG_CHECKING(whether compiler understands -Wall)
old_AM_CFLAGS="$AM_CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_TRY_COMPILE([],[],
               AC_MSG_RESULT(yes),
               AC_MSG_RESULT(no)
               AM_CFLAGS="$old_AM_CFLAGS")

# Checks for library functions.

AC_OUTPUT([Makefile])
