dnl Determine whether compiler supports -Wall
dnl

AC_DEFUN([CC_HAS_WALL],
[
    AC_MSG_CHECKING([whether ${CC-cc} supports -Wall])
    old_CFLAGS="$CFLAGS"
    CFLAGS="-Wall"
    AC_TRY_COMPILE([],[],
        [as_cv_cc_has_wall=yes],
        [as_cv_cc_has_wall=no])
    CFLAGS="$old_CFLAGS"
    if test $as_cv_cc_has_wall = yes; then
        AC_MSG_RESULT([yes])
        AC_SUBST(AM_CFLAGS,"$AM_CFLAGS -Wall")
    else
        AC_MSG_RESULT([no])
    fi
])
