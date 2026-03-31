# Top-level Makefile.am
#
# © 2020-2026 Reuben Thomas <rrt@sc3d.org>

SUBDIRS = libsam test

EMPTY =

loc-local:
	$(CLOC) *.go

EXTRA_DIST = \
	HACKING.md \
	SAM.md \
	go.mod \
	go.sum \
	main.go \
	assembler.go \
	sal.go \
	lexer.go \
	lexer_test.go \
	$(EMPTY)

check-local:
	cd $(srcdir) && go test
