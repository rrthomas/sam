# Top-level & tests Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = libsam

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(srcdir)/run-test

EMPTY =

AM_TESTS_ENVIRONMENT = export srcdir=$(srcdir); \
	cp $(srcdir)/turtle.yaml .;

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
	screen_graphics-expected.log \
	screen_graphics-expected.pbm \
	screen_turtle-demo-expected.log \
	screen_turtle-demo-expected.pbm \
	screen_levy-c-expected.log \
	screen_levy-c-expected.pbm \
	set-expected.log \
	$(EMPTY)

CLOC = cloc --force-lang="C",h

loc:
	$(CLOC) libsam/*.[ch] *.go

EXTRA_DIST = $(YAML_LOG_COMPILER) $(TESTS) $(RESULTS) \
	HACKING.md \
	SAM.md \
	turtle.yaml \
	go.mod \
	go.sum \
	main.go \
	assembler.go \
	$(EMPTY)

clean-local:
	rm -f *-output.log *-output.pbm; \
	if test "$(srcdir)" != "$(builddir)"; then rm -f $(builddir)/turtle.yaml; fi
