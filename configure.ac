AC_INIT([UltraDefrag4Linux], [5.0.0AB.8], [https://github.com/749/UltraDefrag4Linux/issues])
AM_INIT_AUTOMAKE([foreign subdir-objects no-define])
AC_CONFIG_SRCDIR([src/console/defrag.c])

AC_PROG_CC
AC_PROG_GREP
AC_PROG_RANLIB
AM_PROG_AR

AC_ARG_WITH(
    [ntfs3g-includedir],
    [AS_HELP_STRING([--with-ntfs3g-includedir=DIR], [directory containing ntfs-3g headers])],
    [ntfs3g_includedir="$withval"],
    [ntfs3g_includedir=""])

AS_IF([test -z "$ntfs3g_includedir" && test -d /usr/include/ntfs-3g],
    [ntfs3g_includedir="/usr/include/ntfs-3g"])

AS_IF([test -n "$ntfs3g_includedir"],
    [NTFS3G_CFLAGS="-I$ntfs3g_includedir"],
    [NTFS3G_CFLAGS=""])

AS_IF([test -z "$ntfs3g_includedir" || test ! -f "$ntfs3g_includedir/volume.h"],
    [AC_MSG_ERROR([required ntfs-3g headers were not found; install ntfs-3g-dev or use --with-ntfs3g-includedir])])

AC_ARG_WITH(
    [ntfs3g-libdir],
    [AS_HELP_STRING([--with-ntfs3g-libdir=DIR], [directory containing libntfs-3g])],
    [NTFS3G_LDFLAGS="-L$withval"],
    [AS_IF([test -d /usr/lib/x86_64-linux-gnu],
        [NTFS3G_LDFLAGS="-L/usr/lib/x86_64-linux-gnu"],
        [NTFS3G_LDFLAGS=""])])

save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS $NTFS3G_LDFLAGS"
AC_CHECK_LIB([ntfs-3g], [ntfs_mount],
    [NTFS3G_LIBS="$NTFS3G_LDFLAGS -lntfs-3g"],
    [AC_MSG_ERROR([required ntfs-3g library was not found])])
LDFLAGS=$save_LDFLAGS

AC_CHECK_LIB([pthread], [pthread_create],
    [PTHREAD_LIBS="-lpthread"],
    [AC_MSG_ERROR([required pthread library was not found])])

AC_CHECK_LIB([ncurses], [initscr],
    [CURSES_LIBS="-lncurses"],
    [AC_CHECK_LIB([curses], [initscr],
        [CURSES_LIBS="-lcurses"],
        [AC_MSG_ERROR([required curses/ncurses library was not found])])])

AC_SUBST([NTFS3G_CFLAGS])
AC_SUBST([NTFS3G_LIBS])
AC_SUBST([PTHREAD_LIBS])
AC_SUBST([CURSES_LIBS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
