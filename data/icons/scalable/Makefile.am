size = scalable
actioniconsdir = $(pkgdatadir)/icons/hicolor/$(size)/actions
appiconsdir = $(datadir)/icons/hicolor/$(size)/apps

dist_appicons_DATA = \
	apps/xviewer.svg
dist_actionicons_DATA = \
	actions/xviewer-plugin.svg	  	\
	actions/xviewer-image-gallery.svg  \
	actions/slideshow-play.svg

noinst_DATA = \
	apps/xviewer-onecanvas.svg

EXTRA_DIST = $(noinst_DATA)

-include $(top_srcdir)/git.mk
