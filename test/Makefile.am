# Top-level & tests Make file.am
#
# © 2020-2026 Reuben Thomas <rrt@sc3d.org>

LOG_COMPILER = $(srcdir)/run-test

EMPTY =

AM_TESTS_ENVIRONMENT = export top_srcdir=$(top_srcdir); \
	cp $(srcdir)/turtle.sal .;

TESTS = \
	average.yaml \
	basic_block.yaml \
	double_list.yaml \
	map.yaml \
	quote.yaml \
	screen_graphics.yaml \
	set.yaml \
	stack.yaml \
	string.yaml \
	ackermann.sal \
	double_list.sal \
	empty_loop.sal \
	expr.sal \
	factorial_iter.sal \
	factorial_rec.sal \
	fizzbuzz.sal \
	iter.sal \
	map.sal \
	mutated_capture.sal \
	repeated_closure.sal \
	screen_graphics.sal \
	screen_levy-c.sal \
	screen_turtle-demo.sal \
	sum_evens.sal \
	two_double_closures.sal \
	two_closures.sal \
	$(EMPTY)

RESULTS = \
	average.yaml-expected.log \
	basic_block.yaml-expected.log \
	double_list.yaml-expected.log \
	map.yaml-expected.log \
	quote.yaml-expected.log \
	screen_graphics.yaml-expected.log \
	screen_graphics.yaml-expected.pbm \
	set.yaml-expected.log \
	stack.yaml-expected.log \
	string.yaml-expected.log \
	ackermann.sal-expected.log \
	double_list.sal-expected.log \
	empty_loop.sal-expected.log \
	expr.sal-expected.log \
	factorial_iter.sal-expected.log \
	factorial_rec.sal-expected.log \
	fizzbuzz.sal-expected.log \
	iter.sal-expected.log \
	map.sal-expected.log \
	mutated_capture.sal-expected.log \
	repeated_closure.sal-expected.log \
	screen_graphics.sal-expected.log \
	screen_graphics.sal-expected.pbm \
	screen_levy-c.sal-expected.log \
	screen_levy-c.sal-expected.pbm \
	screen_turtle-demo.sal-expected.log \
	screen_turtle-demo.sal-expected.pbm \
	sum_evens.sal-expected.log \
	two_double_closures.sal-expected.log \
	two_closures.sal-expected.log \
	$(EMPTY)

EXTRA_DIST = $(LOG_COMPILER) $(TESTS) $(RESULTS) \
	turtle.sal \
	keyboard.sal \
	audio.sal \
	$(EMPTY)

clean-local:
	rm -f *-fixed.log *-output.log *-output.pbm; \
	if test "$(srcdir)" != "$(builddir)"; then rm -f $(builddir)/turtle.sal; fi
