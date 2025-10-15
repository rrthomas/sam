# Tests Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = doc

AM_VALAFLAGS = --debug --vapidir=$(srcdir) --vapidir=$(srcdir)/vala-extra-vapis
VAPIS =	config.vapi sam.vapi

noinst_LTLIBRARIES = libsam.la

AM_CPPFLAGS = -DSAM_DEBUG

bin_PROGRAMS = sam
sam_SOURCES = main.vala
sam_CFLAGS = $(AM_CFLAGS) -w --include config.h $(GLIB_CFLAGS) $(YAML_CFLAGS)
sam_VALAFLAGS = $(AM_VALAFLAGS) --pkg sdl2 --pkg yaml-0.1 --pkg config --pkg sam
sam_LDADD = $(LDADD) $(GLIB_LIBS) $(SDL_LIBS) $(YAML_LIBS)

libsam_la_LIBADD = $(LTLIBOBJS) $(SDL2_LIBS) -lm
libsam_la_SOURCES = \
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

EMPTY =

TESTS = \
	basic_block.yaml \
	factorial_iter.yaml \
	factorial_rec.yaml \
	push.yaml \
	swap.yaml \
	screen_graphics.yaml \
	screen_turtle-demo.yaml \
	screen_levy-c.yaml \
	$(EMPTY)

RESULTS = \
	basic_block-expected.log \
	factorial_iter-expected.log \
	factorial_rec-expected.log \
	graphics-expected.log \
	push-expected.log \
	swap-expected.log \
	turtle-demo.log \
	levy-c.log \
	$(EMPTY)

CLOC = cloc --force-lang="C",h

loc:
	$(CLOC) $(libsam_la_SOURCES) $(sam_SOURCES)

EXTRA_DIST = $(YAML_LOG_COMPILER) $(TESTS) $(RESULTS) \
	turtle.yaml

AM_TESTS_ENVIRONMENT = export SAMC=$(builddir)/samc;

clean-local:
	rm -f *-output.log
