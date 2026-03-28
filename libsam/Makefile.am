# libsam Makefile.am
#
# © 2020-2026 Reuben Thomas <rrt@sc3d.org>

EMPTY =

noinst_LTLIBRARIES = libsam.la

AM_CPPFLAGS = -DSAM_DEBUG
AM_CFLAGS = -Wall -Wextra -Wno-strict-aliasing

include_HEADERS = \
	sam.h \
	sam_opcodes.h \
	sam_sdl.h

libsam_la_CFLAGS = $(SDL2_CFLAGS) $(LIBGRAPHEME_CFLAGS) $(AM_CFLAGS)
libsam_la_LIBADD = $(LTLIBOBJS) $(SDL2_LIBS) $(LIBGRAPHEME_LIBS) -lm
libsam_la_SOURCES = \
	$(include_HEADERS) \
	private.h \
	run.h \
	blob.c \
	debug.c \
	iter.c \
	map.c \
	stack.c \
	string.c \
	run.c \
	sdl.c \
	state.c \
	traps_basic.h \
	traps_basic.c \
	traps_math.h \
	traps_math.c \
	traps_graphics.h \
	traps_input.h \
	traps_audio.h \
	traps_string.h \
	traps_string.c \
	$(EMPTY)

loc-local:
	$(CLOC) $(libsam_la_SOURCES) *.go

EXTRA_DIST = \
	sam.go \
	verstable.h \
	$(EMPTY)
