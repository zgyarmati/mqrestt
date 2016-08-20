EXTRA_DIST = mqrestt.asciidoc

if HAVE_A2X
dir = $(top_srcdir)/man

man_MANS = mqrestt.1

mqrestt.1:
	$(A2X) -k -f manpage mqrestt.asciidoc

mqrestt.pdf: mqrestt.1
	$(A2X) -f pdf mqrestt.asciidoc -a revdate="`date`" -v -v

CLEANFILES = mqrestt.1 mqrestt.pdf

all-local: mqrestt.pdf mqrestt.1
clean-local:
	rm -rf $(dir)/mqrestt.1
	rm -rf mqrestt.pdf
endif
