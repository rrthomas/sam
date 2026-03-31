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
	nanovg.h \
	nanovg_gl.h \
	nanovg_gl_utils.h \
	nanovg_sw.h \
	nanovg_sw_utils.h \
	nanovg_vtex.h \
	fontstash.h \
	stb_image.h \
	stb_image_write.h \
	stb_truetype.h \
	nanovg.c \
	private.h \
	sdl_private.h \
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
	NotoColorEmoji.ttf \
	NotoEmoji-Regular.ttf \
	NotoSansMono-Regular.ttf \
	NotoSansMono-Bold.ttf \
	NotoSans-Regular.ttf \
	NotoSans-Italic.ttf \
	NotoSans-Bold.ttf \
	NotoSans-BoldItalic.ttf \
	applause.wav \
	beep.wav \
	bell.wav \
	cow.wav \
	explosion.wav \
	gong.wav \
	horse.wav \
	laser.wav \
	oops.wav \
	$(EMPTY)
