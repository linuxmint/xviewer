if ENABLE_JPEG
jpeg_LIB = $(top_builddir)/jpegutils/libxviewer-jpegutils.la
endif

toolbar_LIB = $(top_builddir)/cut-n-paste/toolbar-editor/libtoolbareditor.la

pkglib_LTLIBRARIES = libxviewer.la

bin_PROGRAMS = xviewer

headerdir = $(includedir)/xviewer-@XVIEWER_API_VERSION@/xviewer
header_DATA = $(INST_H_FILES)

MARSHAL_OUTPUT = 	\
	xviewer-marshal.h	\
	xviewer-marshal.c

NOINST_H_FILES =			\
	xviewer-application-internal.h	\
	xviewer-close-confirmation-dialog.h	\
	xviewer-config-keys.h		\
	xviewer-error-message-area.h	\
	xviewer-image-jpeg.h		\
	xviewer-image-private.h		\
	xviewer-metadata-sidebar.h		\
	xviewer-metadata-reader.h		\
	xviewer-metadata-reader-jpg.h	\
	xviewer-metadata-reader-png.h	\
	xviewer-pixbuf-util.h		\
	xviewer-plugin-engine.h             \
	xviewer-preferences-dialog.h	\
	xviewer-print.h			\
	xviewer-print-image-setup.h         \
	xviewer-print-preview.h             \
	xviewer-save-as-dialog-helper.h	\
	xviewer-session.h			\
	xviewer-util.h			\
	zoom.h

INST_H_FILES =				\
	xviewer-application.h		\
	xviewer-application-activatable.h	\
	xviewer-clipboard-handler.h		\
	xviewer-debug.h			\
	xviewer-enums.h			\
	xviewer-file-chooser.h		\
	xviewer-image.h			\
	xviewer-image-save-info.h		\
	xviewer-job-scheduler.h		\
	xviewer-jobs.h			\
	xviewer-list-store.h		\
	xviewer-properties-dialog.h		\
	xviewer-scroll-view.h		\
	xviewer-sidebar.h			\
	xviewer-statusbar.h			\
	xviewer-thumbnail.h			\
	xviewer-thumb-nav.h			\
	xviewer-thumb-view.h		\
	xviewer-transform.h			\
	xviewer-uri-converter.h		\
	xviewer-window-activatable.h	\
	xviewer-window.h

libxviewer_c_files = 			\
	xviewer-application.c		\
	xviewer-application-activatable.c	\
	xviewer-clipboard-handler.c		\
	xviewer-close-confirmation-dialog.c \
	xviewer-debug.c			\
	xviewer-error-message-area.c	\
	xviewer-file-chooser.c		\
	xviewer-image.c			\
	xviewer-image-jpeg.c		\
	xviewer-image-save-info.c		\
	xviewer-job-scheduler.c		\
	xviewer-jobs.c			\
	xviewer-list-store.c		\
	xviewer-metadata-sidebar.c		\
	xviewer-metadata-reader.c		\
	xviewer-metadata-reader-jpg.c	\
	xviewer-metadata-reader-png.c	\
	xviewer-pixbuf-util.c		\
	xviewer-plugin-engine.c            	\
	xviewer-preferences-dialog.c	\
	xviewer-print.c			\
	xviewer-print-image-setup.c         \
	xviewer-print-preview.c           	\
	xviewer-properties-dialog.c		\
	xviewer-save-as-dialog-helper.c	\
	xviewer-scroll-view.c		\
	xviewer-session.c			\
	xviewer-sidebar.c			\
	xviewer-statusbar.c			\
	xviewer-thumbnail.c			\
	xviewer-thumb-nav.c			\
	xviewer-thumb-view.c		\
	xviewer-transform.c			\
	xviewer-uri-converter.c		\
	xviewer-util.c			\
	xviewer-window-activatable.c	\
	xviewer-window.c			\
	zoom.c

libxviewer_la_SOURCES =			\
	$(libxviewer_c_files)		\
	$(BUILT_SOURCES)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if HAVE_EXIF
INST_H_FILES +=				\
	xviewer-exif-util.h

NOINST_H_FILES +=			\
	xviewer-metadata-details.h

libxviewer_c_files +=			\
	xviewer-metadata-details.c		\
	xviewer-exif-util.c
endif

if HAVE_EXEMPI
# We need to make sure xviewer-metadata-details
# is only listed once
# or the build will break with automake-1.11
if !HAVE_EXIF
NOINST_H_FILES +=				\
	xviewer-metadata-details.h

libxviewer_la_SOURCES +=			\
	xviewer-metadata-details.c
endif !HAVE_EXIF
endif HAVE_EXEMPI

libxviewer_la_CPPFLAGS =						\
	-I$(top_srcdir)/jpegutils				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-DG_LOG_DOMAIN=\"XVIEWER\"					\
	-DXVIEWER_PREFIX=\""${prefix}"\"				\
	-DXVIEWER_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DXVIEWER_LOCALE_DIR=\""$(datadir)/locale"\"		\
	-DXVIEWER_PIXMAPS_DIR=\""$(datadir)/pixmaps/xviewer"\"		\
	-DXVIEWER_PLUGIN_DIR=\""$(libdir)/xviewer/plugins"\"		\
	-DLIBDIR=\""$(libdir)"\"

libxviewer_la_CFLAGS =						\
	$(XVIEWER_CFLAGS)						\
	$(WARN_CFLAGS)

libxviewer_la_LIBADD = \
	$(XVIEWER_LIBS)	\
	$(toolbar_LIB)	\
	$(jpeg_LIB)	\
	$(LIBJPEG)	\
	$(LIBM)

if HAVE_LCMS
libxviewer_la_CFLAGS += 	\
	$(X11_CFLAGS)

libxviewer_la_LIBADD += \
	$(X11_LIBS)
endif

libxviewer_la_LDFLAGS = -avoid-version -export-dynamic -no-undefined -export-symbols-regex "^xviewer_.*"

xviewer_SOURCES = main.c

xviewer_CFLAGS = 							\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	$(XVIEWER_CFLAGS)						\
	-DXVIEWER_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DXVIEWER_LOCALE_DIR=\""$(datadir)/locale"\"                \
	$(INTROSPECTION_CFLAGS)

xviewer_LDADD = 				\
	libxviewer.la			\
	$(XVIEWER_LIBS)			\
	$(INTROSPECTION_LIBS)

xviewer_LDFLAGS = -export-dynamic -no-undefined

BUILT_SOURCES = 			\
	xviewer-enum-types.c		\
	xviewer-enum-types.h		\
	xviewer-resources.c			\
	$(MARSHAL_OUTPUT)

xviewer-resources.c: xviewer.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies xviewer.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) "$<" --target=$@ \
         --sourcedir=$(top_srcdir)/data --c-name _xviewer --generate-source

xviewer-enum-types.h: xviewer-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template xviewer-enum-types.h.template $(INST_H_FILES)) > $@

xviewer-enum-types.c: xviewer-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template xviewer-enum-types.c.template $(INST_H_FILES)) > $@

xviewer-marshal.h: xviewer-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --internal --prefix=xviewer_marshal > $@

xviewer-marshal.c: xviewer-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --body --header --prefix=xviewer_marshal > $@

EXTRA_DIST = 				\
	xviewer.gresource.xml		\
	xviewer-enum-types.h.template	\
	xviewer-enum-types.c.template	\
	xviewer-marshal.list

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Xviewer-3.0.gir
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all $(libxviewer_la_CPPFLAGS)

Xviewer-3.0.gir: xviewer
Xviewer_3_0_gir_INCLUDES = Gtk-3.0
Xviewer_3_0_gir_FILES = $(INST_H_FILES) $(libxviewer_c_files) $(BUILT_SOURCES)
Xviewer_3_0_gir_VERSION = 3.0
Xviewer_3_0_gir_PROGRAM = $(builddir)/xviewer
Xviewer_3_0_gir_NAMESPACE = Xviewer
Xviewer_3_0_gir_CFLAGS = $(XVIEWER_CFLAGS)
girdir = $(datadir)/xviewer/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/xviewer/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
