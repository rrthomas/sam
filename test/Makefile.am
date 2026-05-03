# Top-level & tests Make file.am
#
# © 2020-2026 Reuben Thomas <rrt@sc3d.org>

LOG_COMPILER = $(srcdir)/run-test

EMPTY =

AM_TESTS_ENVIRONMENT = export top_srcdir=$(top_srcdir); \
	cp $(srcdir)/turtle.sal .;

TESTS = \
	ackermann.sal \
	average.sal \
	basic_block.sal \
	double_list.sal \
	empty_loop.sal \
	expr.sal \
	factorial_iter.sal \
	factorial_rec.sal \
	fizzbuzz.sal \
	iter.sal \
	map.sal \
	map_asm.sal \
	mutated_capture.sal \
	quote.sal \
	repeated_closure.sal \
	screen_graphics.sal \
	screen_levy-c.sal \
	screen_turtle-demo.sal \
	set.sal \
	stack.sal \
	string.sal \
	sum_evens.sal \
	two_double_closures.sal \
	two_closures.sal \
	$(EMPTY)

RESULTS = \
	ackermann.sal-expected.log \
	average.sal-expected.log \
	basic_block.sal-expected.log \
	double_list.sal-expected.log \
	empty_loop.sal-expected.log \
	expr.sal-expected.log \
	factorial_iter.sal-expected.log \
	factorial_rec.sal-expected.log \
	fizzbuzz.sal-expected.log \
	iter.sal-expected.log \
	map.sal-expected.log \
	map_asm.sal-expected.log \
	mutated_capture.sal-expected.log \
	quote.sal-expected.log \
	repeated_closure.sal-expected.log \
	screen_graphics.sal-expected.log \
	screen_graphics.sal-expected.pbm \
	screen_levy-c.sal-expected.log \
	screen_levy-c.sal-expected.pbm \
	screen_turtle-demo.sal-expected.log \
	screen_turtle-demo.sal-expected.pbm \
	set.sal-expected.log \
	stack.sal-expected.log \
	string.sal-expected.log \
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
