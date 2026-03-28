# Top-level & tests Make file.am
#
# © 2020-2026 Reuben Thomas <rrt@sc3d.org>

LOG_COMPILER = $(srcdir)/run-test

EMPTY =

AM_TESTS_ENVIRONMENT = export top_srcdir=$(top_srcdir); \
	cp $(srcdir)/turtle.yaml .;

TESTS = \
	average.yaml \
	basic_block.yaml \
	double_list.yaml \
	factorial_iter.yaml \
	factorial_rec.yaml \
	map.yaml \
	quote.yaml \
	screen_graphics.yaml \
	screen_turtle-demo.yaml \
	screen_levy-c.yaml \
	set.yaml \
	stack.yaml \
	string.yaml \
	double_list.sal \
	expr.sal \
	factorial_iter.sal \
	factorial_rec.sal \
	iter.sal \
	map.sal \
	mutated_capture.sal \
	repeated_closure.sal \
	screen_graphics.sal \
	sum_evens.sal \
	two_double_closures.sal \
	two_closures.sal \
	$(EMPTY)

RESULTS = \
	average.yaml-expected.log \
	basic_block.yaml-expected.log \
	double_list.yaml-expected.log \
	factorial_iter.yaml-expected.log \
	factorial_rec.yaml-expected.log \
	map.yaml-expected.log \
	quote.yaml-expected.log \
	screen_graphics.yaml-expected.log \
	screen_graphics.yaml-expected.pbm \
	screen_turtle-demo.yaml-expected.log \
	screen_turtle-demo.yaml-expected.pbm \
	screen_levy-c.yaml-expected.log \
	screen_levy-c.yaml-expected.pbm \
	set.yaml-expected.log \
	stack.yaml-expected.log \
	string.yaml-expected.log \
	double_list.sal-expected.log \
	expr.sal-expected.log \
	factorial_iter.sal-expected.log \
	factorial_rec.sal-expected.log \
	iter.sal-expected.log \
	map.sal-expected.log \
	mutated_capture.sal-expected.log \
	repeated_closure.sal-expected.log \
	screen_graphics.sal-expected.log \
	screen_graphics.sal-expected.pbm \
	sum_evens.sal-expected.log \
	two_double_closures.sal-expected.log \
	two_closures.sal-expected.log \
	$(EMPTY)

EXTRA_DIST = $(LOG_COMPILER) $(TESTS) $(RESULTS) \
	turtle.yaml \
	keyboard.sal \
	audio.sal \
	$(EMPTY)

clean-local:
	rm -f *-fixed.log *-output.log *-output.pbm; \
	if test "$(srcdir)" != "$(builddir)"; then rm -f $(builddir)/turtle.yaml; fi
