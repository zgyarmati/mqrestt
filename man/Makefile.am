EXTRA_DIST = mqrestt.md

if HAVE_PANDOC
dir = $(top_srcdir)/man

MANNAME=mqrestt

man_MANS = mqrestt.1

mqrestt.1:
#	$(PANDOC) -k -f manpage mqrestt.asciidoc
	$(PANDOC) --standalone --to man $(MANNAME).md -o $(MANNAME).1

mqrestt.pdf: mqrestt.1
#	$(PANDOC) -f pdf mqrestt.asciidoc -a revdate="`date`" -v -v
	$(PANDOC) --standalone  $(MANNAME).md -t latex  -o $(MANNAME).pdf

CLEANFILES = mqrestt.1 mqrestt.pdf

all-local: mqrestt.pdf mqrestt.1
clean-local:
	rm -rf $(dir)/mqrestt.1
	rm -rf mqrestt.pdf
endif
