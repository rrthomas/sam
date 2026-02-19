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

# Check word size
AC_CHECK_SIZEOF([size_t])
SIZEOF_SIZE_T=$ac_cv_sizeof_size_t
AS_IF([test "$SIZEOF_SIZE_T" != 4 -a "$SIZEOF_SIZE_T" != 8],
   AC_MSG_ERROR([sizeof(size_t) must be 4 or 8!]))
AC_SUBST([SIZEOF_SIZE_T])

# Checks for libraries
PKG_CHECK_MODULES(SDL2, [sdl2 SDL2_gfx])

# Generate output
AC_CONFIG_FILES([Makefile libsam/Makefile test/Makefile])
AC_OUTPUT
