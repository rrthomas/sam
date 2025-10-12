# Tests Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = doc

lib_LTLIBRARIES = libsam.la

AM_CPPFLAGS = -DSAM_DEBUG

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
	$(CLOC) $(libsam_la_SOURCES)

EXTRA_DIST = $(YAML_LOG_COMPILER) $(TESTS) $(RESULTS) \
	turtle.yaml

AM_TESTS_ENVIRONMENT = export SAMC=$(builddir)/samc;

clean-local:
	rm -f *-output.log

python-build: libsam.la
	ctypesgen -o sam/sam_ctypes.py -L $(builddir)/.libs -lsam sam.h sam_private.h
	python -m build

python-dist:
	git diff --exit-code && \
	rm -rf ./dist && \
	mkdir dist && \
	$(MAKE) build

python-release:
	$(MAKE) test && \
	$(MAKE) dist && \
	twine upload dist/* && \
	git tag v$$(grep version pyproject.toml | grep -o "[0-9.]\+") && \
	git push --tags
