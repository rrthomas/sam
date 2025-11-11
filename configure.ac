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

# Checks for libraries.
PKG_CHECK_MODULES(SDL2, [sdl2 SDL2_gfx])

# Generate output
AC_CONFIG_FILES([Makefile libsam/Makefile])
AC_OUTPUT
