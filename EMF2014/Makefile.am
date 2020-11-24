# Tests Makefile.am
#
# Copyright (C) Reuben Thomas 2020

TEST_EXTENSIONS = .yaml
YAML_LOG_COMPILER = $(srcdir)/run-test

EMPTY =

TESTS = \
	basic_block.yaml \
	factorial_iter.yaml \
	factorial_rec.yaml \
	graphics.yaml \
	push.yaml \
	swap.yaml \
	$(EMPTY)

RESULTS = \
	basic_block-expected.log \
	factorial_iter-expected.log \
	factorial_rec-expected.log \
	graphics-expected.log \
	push-expected.log \
	swap-expected.log \
	$(EMPTY)

EXTRA_DIST = $(SH_LOG_COMPILER) $(TESTS) $(INPUTS) $(RESULTS)

AM_TESTS_ENVIRONMENT = srcdir=$(srcdir) MKSAM=$(builddir)/mksam SAM=$(builddir)/sam$(EXEEXT);

clean-local:
	rm -f *-output.log
