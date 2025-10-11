# sam configure.ac
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

AC_INIT([sam],[0.1])
AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
PKG_PROG_PKG_CONFIG
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES(SDL2, [sdl2 SDL2_gfx])

AC_CONFIG_FILES([Makefile doc/Makefile])
AC_OUTPUT
