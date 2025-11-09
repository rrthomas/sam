# libsam Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

noinst_LTLIBRARIES = libsam.la

AM_CPPFLAGS = -DSAM_DEBUG

libsam_la_CFLAGS = $(SDL2_CFLAGS)
libsam_la_LIBADD = $(LTLIBOBJS) $(SDL2_LIBS) -lm
libsam_la_SOURCES = \
	minmax.h \
	sam.h \
	sam_private.h \
	sam_opcodes.h \
	sam_registers.h \
	sam_traps.h \
	sam_debug.c \
	sam_storage.c \
	sam_run.c \
	sam_traps.c

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(srcdir)/run-test

EXTRA_DIST = sam.go

CLOC = cloc --force-lang="C",h

loc:
	$(CLOC) $(libsam_la_SOURCES)
