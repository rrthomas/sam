# Tests Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = doc

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(srcdir)/run-test

EMPTY =

TESTS = \
	basic_block.yaml \
	factorial_iter.yaml \
	factorial_rec.yaml \
	set.yaml \
	screen_graphics.yaml \
	screen_turtle-demo.yaml \
	screen_levy-c.yaml \
	$(EMPTY)

RESULTS = \
	basic_block-expected.log \
	factorial_iter-expected.log \
	factorial_rec-expected.log \
	graphics-expected.log \
	set-expected.log \
	turtle-demo.log \
	levy-c.log \
	$(EMPTY)

CLOC = cloc --force-lang="C",h

loc:
	$(CLOC) libsam/*.[ch] *.go

EXTRA_DIST = $(YAML_LOG_COMPILER) $(TESTS) $(RESULTS) \
	turtle.yaml

clean-local:
	rm -f *-output.log
