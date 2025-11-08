# sam configure.ac
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

AC_INIT([sam],[0.1])
AM_INIT_AUTOMAKE([-Wall std-options foreign])

# Checks for programs
AC_PROG_CC
AM_PROG_AR
PKG_PROG_PKG_CONFIG
LT_INIT
AC_PATH_PROG([LATEXMK], [latexmk], [AC_MSG_ERROR([latexmk not found])])

# Generate output
AC_CONFIG_FILES([Makefile libsam/Makefile doc/Makefile])
AC_OUTPUT
