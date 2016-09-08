AUTOMAKE_OPTIONS = subdir-objects

pluginsdir = $(libdir)/xviewer/plugins

AM_CPPFLAGS = \
	-I$(top_srcdir)/src 				\
	$(XVIEWER_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	-DXVIEWER_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_LDFLAGS = -avoid-version -module

plugins_LTLIBRARIES = \
	fullscreen/libfullscreen.la

plugins_in_files = \
	fullscreen/fullscreen.plugin.desktop.in

# Fullscreen plugin
fullscreen_libfullscreen_la_SOURCES = \
	fullscreen/xviewer-fullscreen-plugin.h  \
	fullscreen/xviewer-fullscreen-plugin.c

fullscreen_libfullscreen_la_LIBADD = \
	$(top_builddir)/src/libxviewer.la	\
	$(XVIEWER_LIBS)

# Statusbar Date Plugin
if HAVE_EXIF
plugins_LTLIBRARIES += statusbar-date/libstatusbar-date.la

statusbar_date_libstatusbar_date_la_SOURCES = \
	statusbar-date/xviewer-statusbar-date-plugin.h  \
	statusbar-date/xviewer-statusbar-date-plugin.c

statusbar_date_libstatusbar_date_la_LIBADD = \
	$(top_builddir)/src/libxviewer.la	\
	$(XVIEWER_LIBS)

plugins_in_files += statusbar-date/statusbar-date.plugin.desktop.in
endif

# Generate plugin info files

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(AM_V_GEN)$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugins_DATA = $(plugins_in_files:.plugin.desktop.in=.plugin)

DISTCLEANFILES = $(plugins_DATA)
CLEANFILES = $(plugins_DATA)

EXTRA_DIST = $(plugins_in_files)

-include $(top_srcdir)/git.mk
