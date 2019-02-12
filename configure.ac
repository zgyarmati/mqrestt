AC_PREREQ([2.69])
AC_INIT(mqrestt, 0.2~, zgyarmati@zgyarmati.de)
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset strchr strdup strerror strtol])


# Checks for libraries.
# look for libmosquitto
AC_CHECK_HEADER([mosquitto.h],havelibmosquitto=yes,havelibmosquitto=no)
AC_CHECK_LIB([mosquitto], [mosquitto_lib_init],havelibmosquitto=yes,havelibmosquitto=no)
if test "x$havelibmosquitto" = "xyes"; then
    AC_DEFINE(HAVE_LIBMOSQUITTO, 1, libmosquitto MQTT client lib)
    AC_SUBST(MOSQUITTO_LIBS,-lmosquitto)
else
    AC_MSG_ERROR([No libmosquitto found!])
fi

# Check for libcurl
LIBCURL_CHECK_CONFIG([yes],[],[],[AC_MSG_ERROR([libcurl development files required])])

PKG_PROG_PKG_CONFIG(0.26)
PKG_CHECK_MODULES([libconfuse], [libconfuse])

# check for doc generating tools
AC_CHECK_PROGS([A2X], [a2x])
if test -z "$A2X";then
    AC_MSG_WARN([asciidoc a2x not found - continuing without building man pages])
fi
AM_CONDITIONAL([HAVE_A2X], [test -n "$A2X"])

AC_CONFIG_FILES([Makefile
                 man/Makefile
                 src/Makefile])

AC_OUTPUT
