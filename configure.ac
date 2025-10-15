# sam configure.ac
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

AC_INIT([sam],[0.1])
AM_INIT_AUTOMAKE([-Wall std-options foreign])

# Checks for programs
AC_PROG_CC
AM_PROG_AR
PKG_PROG_PKG_CONFIG
AX_CHECK_GNU_MAKE(,[AC_MSG_WARN([GNU make is required to build from Vala sources])])
LT_INIT

# Vala
AM_PROG_VALAC(0.56,,[AC_MSG_WARN([valac not found or too old: cannot compile Vala sources])])
PKG_CHECK_MODULES(GLIB, [glib-2.0 gobject-2.0])

# Checks for libraries
PKG_CHECK_MODULES(SDL2, [sdl2 SDL2_gfx])
PKG_CHECK_MODULES(YAML, [yaml-0.1])

# Generate output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile doc/Makefile])
AC_OUTPUT
