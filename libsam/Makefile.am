# libsam Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

EMPTY =

noinst_LTLIBRARIES = libsam.la

AM_CPPFLAGS = -DSAM_DEBUG
AM_CFLAGS = -Wall -Wno-strict-aliasing

include_HEADERS = \
	sam.h \
	sam_opcodes.h

libsam_la_CFLAGS = $(SDL2_CFLAGS) $(AM_CFLAGS)
libsam_la_LIBADD = $(LTLIBOBJS) $(SDL2_LIBS) -lm
libsam_la_SOURCES = \
	$(include_HEADERS) \
	private.h \
	run.h \
	debug.c \
	stack.c \
	frame.c \
	run.c \
	traps_math.h \
	traps_math.c \
	traps_graphics.h \
	traps_graphics.c \
	$(EMPTY)

EXTRA_DIST = sam.go
