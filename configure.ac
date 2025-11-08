# sam configure.ac
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

AC_INIT([sam],[0.1])
AM_INIT_AUTOMAKE([-Wall std-options foreign])

# Checks for programs
AC_PATH_PROG([LATEXMK], [latexmk], [AC_MSG_ERROR([latexmk not found])])

# Generate output
AC_CONFIG_FILES([Makefile doc/Makefile])
AC_OUTPUT
