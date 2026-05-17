AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
	-DLXGC=1 \
	$(NTFS3G_CFLAGS) \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/console \
	-I$(top_srcdir)/src/dll/udefrag \
	-I$(top_srcdir)/src/dll/zenwinx \
	-I$(top_srcdir)/src/wincalls

bin_PROGRAMS = udefrag

noinst_HEADERS = \
	src/console/udefrag.h \
	src/include/compiler.h \
	src/include/extrawin.h \
	src/include/linux.h \
	src/include/ultradfgver.h \
	src/include/version.h \
	src/share/getopt.h \
	src/dll/udefrag/udefrag.h \
	src/dll/udefrag/udefrag-internals.h \
	src/dll/zenwinx/myendians.h \
	src/dll/zenwinx/mytypes.h \
	src/dll/zenwinx/ntndk.h \
	src/dll/zenwinx/ntfs.h \
	src/dll/zenwinx/partition.h \
	src/dll/zenwinx/prb.h \
	src/dll/zenwinx/zenwinx.h \
	src/dll/zenwinx/zenwinxver.h \
	src/wincalls/ntfs-3g.h

EXTRA_DIST = autogen.sh

udefrag_SOURCES = \
	src/console/defrag.c \
	src/console/map.c \
	src/console/options.c \
	src/dll/udefrag/analyze.c \
	src/dll/udefrag/defrag.c \
	src/dll/udefrag/map.c \
	src/dll/udefrag/move.c \
	src/dll/udefrag/optimize.c \
	src/dll/udefrag/options.c \
	src/dll/udefrag/reports.c \
	src/dll/udefrag/search.c \
	src/dll/udefrag/tasks.c \
	src/dll/udefrag/udefrag.c \
	src/dll/udefrag/volume.c \
	src/dll/zenwinx/dbg.c \
	src/dll/zenwinx/env.c \
	src/dll/zenwinx/event.c \
	src/dll/zenwinx/file.c \
	src/dll/zenwinx/ftw.c \
	src/dll/zenwinx/ftw_ntfs.c \
	src/dll/zenwinx/ldr.c \
	src/dll/zenwinx/list.c \
	src/dll/zenwinx/lock.c \
	src/dll/zenwinx/mem.c \
	src/dll/zenwinx/misc.c \
	src/dll/zenwinx/path.c \
	src/dll/zenwinx/prb.c \
	src/dll/zenwinx/string.c \
	src/dll/zenwinx/thread.c \
	src/dll/zenwinx/time.c \
	src/dll/zenwinx/volume.c \
	src/dll/zenwinx/zenwinx.c \
	src/wincalls/wincalls.c \
	src/wincalls/ntfs-3g.c \
	src/wincalls/curses.c

udefrag_LDADD = \
	$(NTFS3G_LIBS) \
	$(PTHREAD_LIBS) \
	$(CURSES_LIBS) \
	-lm

check-local: udefrag
	./udefrag --help | $(GREP) -q "Usage: udefrag"
