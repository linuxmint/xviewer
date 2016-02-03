size = scalable
actioniconsdir = $(pkgdatadir)/icons/hicolor/$(size)/actions
appiconsdir = $(datadir)/icons/hicolor/$(size)/apps

dist_appicons_DATA = \
	apps/eog.svg
dist_actionicons_DATA = \
	actions/eog-plugin.svg	  	\
	actions/eog-image-gallery.svg  \
	actions/slideshow-play.svg

noinst_DATA = \
	apps/eog-onecanvas.svg

EXTRA_DIST = $(noinst_DATA)

-include $(top_srcdir)/git.mk
