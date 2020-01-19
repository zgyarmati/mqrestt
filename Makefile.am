AUTOMAKE_OPTIONS = foreign
SUBDIRS = src man
EXTRA_DIST = autogen.sh README.md mqrestt.ini debian tools COPYING

dist-hook:
	if test -d "$(srcdir)/.git" && type git > /dev/null 2>&1; then \
	( cd "$(srcdir)" && git log --pretty=format:"%ad %s" --date=iso ) > "$(distdir)/ChangeLog"; \
	fi

#mqresttconfigdir=$(sysconfdir)

#mqresttconfig_DATA=mqrestt.ini
sysconf_DATA = mqrestt.ini
