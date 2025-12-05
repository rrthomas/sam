# libsam Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

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
	debug.c \
	storage.c \
	run.c \
	traps.c

EXTRA_DIST = sam.go
