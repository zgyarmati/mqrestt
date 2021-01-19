#!/bin/sh

#dependecies
#apt-get install asciidoc dblatex
#edit /etc/asciidoc/dblatex/asciidoc-dblatex.xsl:

#<xsl:param name="doc.publisher.show">0</xsl:param>

MANNAME=mqrestt

a2x -fpdf ${MANNAME}.asciidoc -a revdate="`date`" -v -v
a2x -f manpage ${MANNAME}.asciidoc -v -v
#a2x -f text ${MANNAME}.asciidoc -v -v


