#!/bin/sh

#dependecies
#apt-get install asciidoc dblatex
#edit /etc/asciidoc/dblatex/asciidoc-dblatex.xsl:

#<xsl:param name="doc.publisher.show">0</xsl:param>

MANNAME=mqrestt

pandoc --standalone --to man ${MANNAME}.md -o ${MANNAME}.1
pandoc --standalone  ${MANNAME}.md -t latex  -o ${MANNAME}.pdf
