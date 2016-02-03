if ENABLE_JPEG
jpeg_LIB = $(top_builddir)/jpegutils/libeog-jpegutils.la
endif

toolbar_LIB = $(top_builddir)/cut-n-paste/toolbar-editor/libtoolbareditor.la

pkglib_LTLIBRARIES = libeog.la

bin_PROGRAMS = eog

headerdir = $(includedir)/eog-@EOG_API_VERSION@/eog
header_DATA = $(INST_H_FILES)

MARSHAL_OUTPUT = 	\
	eog-marshal.h	\
	eog-marshal.c

NOINST_H_FILES =			\
	eog-application-internal.h	\
	eog-close-confirmation-dialog.h	\
	eog-config-keys.h		\
	eog-error-message-area.h	\
	eog-image-jpeg.h		\
	eog-image-private.h		\
	eog-metadata-sidebar.h		\
	eog-metadata-reader.h		\
	eog-metadata-reader-jpg.h	\
	eog-metadata-reader-png.h	\
	eog-pixbuf-util.h		\
	eog-plugin-engine.h             \
	eog-preferences-dialog.h	\
	eog-print.h			\
	eog-print-image-setup.h         \
	eog-print-preview.h             \
	eog-save-as-dialog-helper.h	\
	eog-session.h			\
	eog-util.h			\
	zoom.h

INST_H_FILES =				\
	eog-application.h		\
	eog-application-activatable.h	\
	eog-clipboard-handler.h		\
	eog-debug.h			\
	eog-enums.h			\
	eog-file-chooser.h		\
	eog-image.h			\
	eog-image-save-info.h		\
	eog-job-scheduler.h		\
	eog-jobs.h			\
	eog-list-store.h		\
	eog-properties-dialog.h		\
	eog-scroll-view.h		\
	eog-sidebar.h			\
	eog-statusbar.h			\
	eog-thumbnail.h			\
	eog-thumb-nav.h			\
	eog-thumb-view.h		\
	eog-transform.h			\
	eog-uri-converter.h		\
	eog-window-activatable.h	\
	eog-window.h

libeog_c_files = 			\
	eog-application.c		\
	eog-application-activatable.c	\
	eog-clipboard-handler.c		\
	eog-close-confirmation-dialog.c \
	eog-debug.c			\
	eog-error-message-area.c	\
	eog-file-chooser.c		\
	eog-image.c			\
	eog-image-jpeg.c		\
	eog-image-save-info.c		\
	eog-job-scheduler.c		\
	eog-jobs.c			\
	eog-list-store.c		\
	eog-metadata-sidebar.c		\
	eog-metadata-reader.c		\
	eog-metadata-reader-jpg.c	\
	eog-metadata-reader-png.c	\
	eog-pixbuf-util.c		\
	eog-plugin-engine.c            	\
	eog-preferences-dialog.c	\
	eog-print.c			\
	eog-print-image-setup.c         \
	eog-print-preview.c           	\
	eog-properties-dialog.c		\
	eog-save-as-dialog-helper.c	\
	eog-scroll-view.c		\
	eog-session.c			\
	eog-sidebar.c			\
	eog-statusbar.c			\
	eog-thumbnail.c			\
	eog-thumb-nav.c			\
	eog-thumb-view.c		\
	eog-transform.c			\
	eog-uri-converter.c		\
	eog-util.c			\
	eog-window-activatable.c	\
	eog-window.c			\
	zoom.c

libeog_la_SOURCES =			\
	$(libeog_c_files)		\
	$(BUILT_SOURCES)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if HAVE_EXIF
INST_H_FILES +=				\
	eog-exif-util.h

NOINST_H_FILES +=			\
	eog-metadata-details.h

libeog_c_files +=			\
	eog-metadata-details.c		\
	eog-exif-util.c
endif

if HAVE_EXEMPI
# We need to make sure eog-metadata-details
# is only listed once
# or the build will break with automake-1.11
if !HAVE_EXIF
NOINST_H_FILES +=				\
	eog-metadata-details.h

libeog_la_SOURCES +=			\
	eog-metadata-details.c
endif !HAVE_EXIF
endif HAVE_EXEMPI

libeog_la_CPPFLAGS =						\
	-I$(top_srcdir)/jpegutils				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-DG_LOG_DOMAIN=\"EOG\"					\
	-DEOG_PREFIX=\""${prefix}"\"				\
	-DEOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOG_LOCALE_DIR=\""$(datadir)/locale"\"		\
	-DEOG_PIXMAPS_DIR=\""$(datadir)/pixmaps/eog"\"		\
	-DEOG_PLUGIN_DIR=\""$(libdir)/eog/plugins"\"		\
	-DLIBDIR=\""$(libdir)"\"

libeog_la_CFLAGS =						\
	$(EOG_CFLAGS)						\
	$(WARN_CFLAGS)

libeog_la_LIBADD = \
	$(EOG_LIBS)	\
	$(toolbar_LIB)	\
	$(jpeg_LIB)	\
	$(LIBJPEG)	\
	$(LIBM)

if HAVE_LCMS
libeog_la_CFLAGS += 	\
	$(X11_CFLAGS)

libeog_la_LIBADD += \
	$(X11_LIBS)
endif

libeog_la_LDFLAGS = -avoid-version -export-dynamic -no-undefined -export-symbols-regex "^eog_.*"

eog_SOURCES = main.c

eog_CFLAGS = 							\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	$(EOG_CFLAGS)						\
	-DEOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOG_LOCALE_DIR=\""$(datadir)/locale"\"                \
	$(INTROSPECTION_CFLAGS)

eog_LDADD = 				\
	libeog.la			\
	$(EOG_LIBS)			\
	$(INTROSPECTION_LIBS)

eog_LDFLAGS = -export-dynamic -no-undefined

BUILT_SOURCES = 			\
	eog-enum-types.c		\
	eog-enum-types.h		\
	eog-resources.c			\
	$(MARSHAL_OUTPUT)

eog-resources.c: eog.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies eog.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) "$<" --target=$@ \
         --sourcedir=$(top_srcdir)/data --c-name _eog --generate-source

eog-enum-types.h: eog-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eog-enum-types.h.template $(INST_H_FILES)) > $@

eog-enum-types.c: eog-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eog-enum-types.c.template $(INST_H_FILES)) > $@

eog-marshal.h: eog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --internal --prefix=eog_marshal > $@

eog-marshal.c: eog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --body --header --prefix=eog_marshal > $@

EXTRA_DIST = 				\
	eog.gresource.xml		\
	eog-enum-types.h.template	\
	eog-enum-types.c.template	\
	eog-marshal.list

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Eog-3.0.gir
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all $(libeog_la_CPPFLAGS)

Eog-3.0.gir: eog
Eog_3_0_gir_INCLUDES = Gtk-3.0
Eog_3_0_gir_FILES = $(INST_H_FILES) $(libeog_c_files) $(BUILT_SOURCES)
Eog_3_0_gir_VERSION = 3.0
Eog_3_0_gir_PROGRAM = $(builddir)/eog
Eog_3_0_gir_NAMESPACE = Eog
Eog_3_0_gir_CFLAGS = $(EOG_CFLAGS)
girdir = $(datadir)/eog/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/eog/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
