size = 32x32
actioniconsdir = $(pkgdatadir)/icons/hicolor/$(size)/actions
appiconsdir = $(datadir)/icons/hicolor/$(size)/apps

dist_appicons_DATA = \
	apps/xviewer.png
dist_actionicons_DATA = \
	actions/xviewer-plugin.png	  	\
	actions/xviewer-image-gallery.png  \
	actions/slideshow-play.png

noinst_DATA = \
	actions/xviewer-image-gallery.svg  \
	actions/xviewer-plugin.svg  \
	actions/slideshow-play.svg

EXTRA_DIST = $(noinst_DATA)

-include $(top_srcdir)/git.mk
