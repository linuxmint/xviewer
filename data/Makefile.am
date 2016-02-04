SUBDIRS = pixmaps icons

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES= xviewer.desktop.in.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_in_files = xviewer.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

gsettings_ENUM_NAMESPACE = org.gnome.xviewer
gsettings_ENUM_FILES = $(top_srcdir)/src/xviewer-scroll-view.h	\
                       $(top_srcdir)/src/xviewer-window.h
gsettings_SCHEMAS = org.gnome.xviewer.gschema.xml

@GSETTINGS_RULES@

gsettingsconvertdir = $(datadir)/GConf/gsettings
gsettingsconvert_DATA = xviewer.convert

uidir = $(pkgdatadir)
ui_DATA = \
	xviewer-toolbar.xml

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xviewer.pc

EXTRA_DIST =			\
	$(ui_DATA)		\
	xviewer.css			\
	xviewer-app-menu.xml	\
	xviewer-image-properties-dialog.ui	\
	xviewer-multiple-save-as-dialog.ui	\
	xviewer-preferences-dialog.ui \
	xviewer-ui.xml		\
	$(DESKTOP_IN_FILES)	\
	$(appdata_in_files)	\
	xviewer.convert

CLEANFILES = 			\
	$(appdata_DATA)

DISTCLEANFILES = 		\
        $(DESKTOP_FILES)        \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
