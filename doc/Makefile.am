# Docs Makefile.am
#
# (c) Reuben Thomas 2011-2025
#
# The package is distributed under the GNU General Public License version 3,
# or, at your option, any later version.
#
# THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
# RISK.

LATEX_SRCS = sam.tex
LATEX_PDFS = sam.pdf
dist_doc_DATA = $(LATEX_PDFS)

all: pdf

pdf-local: $(LATEX_PDFS)
	$(LATEXMK) $(LATEX_SRCS)

MAINTAINERCLEANFILES = $(LATEX_PDFS)

clean-local:
	$(LATEXMK) -r $(srcdir)/latexmkrc -C -f $(LATEX_SRCS) \
		$(builddir)/sam.tex

EXTRA_DIST = $(LATEX_SRCS) $(LATEX_PDFS) latexmkrc
