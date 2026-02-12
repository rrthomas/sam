# Top-level & tests Makefile.am
#
# © 2020-2025 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = libsam

LOG_COMPILER = $(srcdir)/run-test

EMPTY =

AM_TESTS_ENVIRONMENT = export srcdir=$(srcdir); \
	cp $(srcdir)/turtle.yaml .;

TESTS = \
	average.yaml \
	basic_block.yaml \
	double_list.yaml \
	factorial_iter.yaml \
	factorial_rec.yaml \
	quote.yaml \
	set.yaml \
	screen_graphics.yaml \
	screen_turtle-demo.yaml \
	screen_levy-c.yaml \
	stack.yaml \
	expr.sal \
	graphics.sal \
	factorial_loop.sal \
	sum_evens.sal \
	$(EMPTY)

RESULTS = \
	average-expected.log \
	basic_block-expected.log \
	double_list-expected.log \
	factorial_iter-expected.log \
	factorial_rec-expected.log \
	quote-expected.log \
	screen_graphics-expected.log \
	screen_graphics-expected.pbm \
	screen_turtle-demo-expected.log \
	screen_turtle-demo-expected.pbm \
	screen_levy-c-expected.log \
	screen_levy-c-expected.pbm \
	stack-expected.log \
	set-expected.log \
	expr-expected.log \
	graphics-expected.log \
	factorial_loop-expected.log \
	sum_evens-expected.log \
	$(EMPTY)

CLOC = cloc --force-lang="C",h

loc:
	$(CLOC) libsam/*.[ch] libsam/*.go *.go

EXTRA_DIST = $(LOG_COMPILER) $(TESTS) $(RESULTS) \
	HACKING.md \
	SAM.md \
	turtle.yaml \
	go.mod \
	go.sum \
	main.go \
	assembler.go \
	sal.go \
	lexer.go \
	lexer_test.go \
	$(EMPTY)

clean-local:
	rm -f *-fixed.log *-output.log *-output.pbm; \
	if test "$(srcdir)" != "$(builddir)"; then rm -f $(builddir)/turtle.yaml; fi

check-local:
	cd $(srcdir) && go test
