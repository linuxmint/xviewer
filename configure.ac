AC_PREREQ(2.59)

m4_define(xviewer_major_version,  3)
m4_define(xviewer_minor_version, 0)
m4_define(xviewer_micro_version,  0)
m4_define(xviewer_version, xviewer_major_version.xviewer_minor_version.xviewer_micro_version)

AC_INIT([xviewer], xviewer_version, [https://github.com/linuxmint/xviewer/issues], [xviewer])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AC_CONFIG_MACRO_DIR([m4])

# Support silencing the build output if supported (automake-1.11+)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_DEFINE(XVIEWER_MAJOR_VERSION, xviewer_major_version, [XVIEWER major version])
AC_SUBST(XVIEWER_MAJOR_VERSION,  xviewer_major_version)
AC_DEFINE(XVIEWER_MINOR_VERSION, xviewer_minor_version, [XVIEWER minor version])
AC_SUBST(XVIEWER_MINOR_VERSION,  xviewer_minor_version)
AC_DEFINE(XVIEWER_MICRO_VERSION, xviewer_micro_version, [XVIEWER micro version])
AC_SUBST(XVIEWER_MICRO_VERSION,  xviewer_micro_version)

XVIEWER_API_VERSION=3.0
AC_SUBST(XVIEWER_API_VERSION)

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE([enable])

GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS
GNOME_DEBUG_CHECK

# Disable building static archives by default
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

PKG_PROG_PKG_CONFIG

GTK_DOC_CHECK(1.16)

AC_CHECK_FUNCS(strptime)

# ****************************************************************
# Support for nl_langinfo (_NL_MEASUREMENT_MEASUREMENT) (optional)
# ****************************************************************
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <langinfo.h>]],
                                [[
  char c;
  c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));]])],
               [have_langinfo_measurement=yes], [have_langinfo_measurement=no])
AC_MSG_RESULT($have_langinfo_measurement)
if test "$have_langinfo_measurement" = "yes"; then
  AC_DEFINE([HAVE__NL_MEASUREMENT_MEASUREMENT], [1],
            [Define if _NL_MEASUREMENT_MEASUREMENT is available])
fi

# ***********
# Translation
# ***********

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.50.1])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

# ***************
# User documentation
# ***************
YELP_HELP_INIT

# **********
# GNOME Libs
# **********

GTK_REQUIRED=3.10.0
GLIB_REQUIRED=2.38.0
CINNAMON_DESKTOP_REQUIRED=3.2.0
GDKPIXBUF_REQUIRED=2.4.0
GTK_PRINT_REQUIRED=3.5.4
SHARED_MIME_INFO_REQUIRED=0.20
EXEMPI_REQUIRED=1.99.5
LIBPEAS_REQUIRED=0.7.4
LIBPEAS_GTK_REQUIRED=0.7.4
XAPP_REQUIRED=1.9.0

XVIEWER_MODULES="gtk+-3.0 >= $GTK_REQUIRED \
             glib-2.0 >= $GLIB_REQUIRED \
             gio-2.0 >= $GLIB_REQUIRED \
             gio-unix-2.0 >= $GLIB_REQUIRED \
             cinnamon-desktop >= $CINNAMON_DESKTOP_REQUIRED \
             gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED \
             gtk+-unix-print-3.0 >= $GTK_PRINT_REQUIRED \
             shared-mime-info >= $SHARED_MIME_INFO_REQUIRED \
             libpeas-1.0 >= $LIBPEAS_REQUIRED \
             libpeas-gtk-1.0 >= $LIBPEAS_GTK_REQUIRED \
             xapp >= $XAPP_REQUIRED"

# Introspection
GOBJECT_INTROSPECTION_CHECK([0.9.3])

if test "$found_introspection" = "yes"; then
  have_introspection=yes
  AC_DEFINE([HAVE_INTROSPECTION], [1], [Define to enable GObject Introspection])
else
  have_introspection=no
fi

# ***************
# libm (required)
# ***************

LT_LIB_M

# ***************
# ZLIB (required)
# ***************

have_zlib=yes
AC_CHECK_HEADERS([zlib.h],
		 [AC_CHECK_LIB([z], [inflate],
			       [AC_CHECK_LIB([z], [crc32], [],
					     [have_zlib=no])],
			       [have_zlib=no])],
		 [have_zlib=no])

if test x$have_zlib = xno; then
  AC_MSG_ERROR([No sufficient zlib library found on your system.])
fi

# ***************
# EXIF (optional)
# ***************

LIBEXIF_REQUIRED=0.6.14

AC_ARG_WITH([libexif], AS_HELP_STRING([--without-libexif], [disable EXIF support]))
have_exif=no
if test x$with_libexif != xno; then
  PKG_CHECK_MODULES(EXIF, libexif >= $LIBEXIF_REQUIRED, have_exif=yes, have_exif=no)
  if test "x$have_exif" = "xyes"; then
    AC_CHECK_HEADER([libexif/exif-data.h],[],[have_exif=no])
  fi
fi
if test "x$have_exif" = "xyes"; then
  AC_DEFINE(HAVE_EXIF, 1, [EXIF Support.])
  XVIEWER_MODULES="$XVIEWER_MODULES libexif >= $LIBEXIF_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXIF], [test "x$have_exif" = "xyes"])

# *********************
# Little CMS (optional)
# *********************

AC_ARG_WITH([cms], AS_HELP_STRING([--without-cms], [disable colour management support]))
have_lcms=no
if test x$with_cms != xno; then
  PKG_CHECK_MODULES(LCMS, lcms2, have_lcms=yes, have_lcms=no)
fi
if test "x$have_lcms" = "xyes"; then
  AC_DEFINE(HAVE_LCMS, 1, [Little CMS Support.])
  XVIEWER_MODULES="$XVIEWER_MODULES lcms2"
fi

AM_CONDITIONAL([HAVE_LCMS], [test "x$have_lcms" = "xyes"])

# *********************
# Exempi (optional)
# *********************
AC_ARG_WITH(xmp, AS_HELP_STRING([--without-xmp], [disable special XMP support]))
have_exempi=no
if test x$with_xmp != xno; then
  PKG_CHECK_MODULES(EXEMPI, exempi-2.0 >= $EXEMPI_REQUIRED, have_exempi=yes, have_exempi=no)
fi
if test "x$have_exempi" = "xyes"; then
  AC_DEFINE(HAVE_EXEMPI,1, [XMP support.])
  XVIEWER_MODULES="$XVIEWER_MODULES exempi-2.0 >= $EXEMPI_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXEMPI], [test "x$have_exempi" = "xyes"])

# Build EXIF-related documentation if we really have support for it
if test "x$have_exif" = "xyes" || test "x$have_exempi" = "xyes"; then
  XVIEWER_DOC_EXIF_START=""
  XVIEWER_DOC_EXIF_STOP=""
else
  XVIEWER_DOC_EXIF_START="<!--"
  XVIEWER_DOC_EXIF_STOP="-->"
fi

AC_SUBST([XVIEWER_DOC_EXIF_START])
AC_SUBST([XVIEWER_DOC_EXIF_STOP])

# ********************
# Jpeg (semi-optional)
# ********************

AC_ARG_WITH(libjpeg, AS_HELP_STRING([--without-libjpeg], [disable special JPEG support]))
have_jpeg=no
have_libjpeg_80=no
if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
  AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
    have_jpeg=yes,
    have_jpeg=no
    AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***))
  if test "$have_jpeg" = yes; then
    AC_MSG_CHECKING([for jpeglib.h])
    AC_PREPROC_IFELSE([AC_LANG_SOURCE(
                                      [[
#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>
                                       ]])],
                      [have_jpeg=yes], [have_jpeg=no])
    AC_MSG_RESULT($have_jpeg)
    if test "$have_jpeg" = "yes"; then
      LIBJPEG='-ljpeg'
      AC_DEFINE(HAVE_JPEG, 1, [Jpeg Support.])
      AC_DEFINE(HAVE_LIBJPEG, 1, [libjpeg is Present.])
      AC_CHECK_LIB(jpeg, jpeg_simple_progression,
        AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1, [Jpeg Progressive Saving Support.]),
        AC_MSG_WARN(JPEG library does not support progressive saving.))
	AC_MSG_CHECKING([[libjpeg version is 8 or greater]])
	AC_PREPROC_IFELSE([ AC_LANG_SOURCE(
					   [[
#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif					   ]])],
			  [AC_MSG_RESULT(yes)
			   have_libjpeg_80=yes],
		          [AC_MSG_RESULT(no)
		           have_libjpeg_80=no])
    else
      AC_MSG_WARN(*** JPEG loader will not be built (JPEG header file not found) ***)
    fi
  fi
fi

if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
  AC_MSG_ERROR([
*** Checks for JPEG loader failed. You can build without it by passing
*** --without-libjpeg to configure but some programs using GTK+ may
*** not work properly])
fi
AC_SUBST(LIBJPEG)
AM_CONDITIONAL(ENABLE_JPEG, test x$have_jpeg = xyes)
AM_CONDITIONAL(HAVE_LIBJPEG_80, test "x$have_libjpeg_80" = xyes)

# ************************************
# libXML2 (required for toolbareditor)
# ************************************

LIBXML2_REQUIRED=2.0
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= $LIBXML2_REQUIRED])

# ***************
# RSVG (optional for scaling svg image)
# ***************

LIBRSVG_REQUIRED=2.36.2

AC_ARG_WITH([librsvg], AS_HELP_STRING([--without-librsvg], [disable RSVG support]))
have_rsvg=no
if test x$with_librsvg != xno; then
  PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= $LIBRSVG_REQUIRED, have_rsvg=yes, have_rsvg=no)
fi
if test "x$have_rsvg" = "xyes"; then
  AC_DEFINE(HAVE_RSVG, 1, [RSVG Support.])
  XVIEWER_MODULES="$XVIEWER_MODULES librsvg-2.0 >= $LIBRSVG_REQUIRED"
fi

AM_CONDITIONAL([HAVE_RSVG], [test "x$have_rsvg" = "xyes"])

# ****************
# CFLAGS/LIBS init
# ****************

PKG_CHECK_MODULES(XVIEWER, $XVIEWER_MODULES)
AC_SUBST(XVIEWER_CFLAGS)
AC_SUBST(XVIEWER_LIBS)

# *******************************************************
# libX11 (required for TotemScrSaver and Color Profiling)
# *******************************************************

# Check GDK rendering target
gdk_target=`$PKG_CONFIG --variable=targets gdk-3.0`

if test "x$have_lcms" = "xyes" || test "$gdk_target" = "x11"; then
  PKG_CHECK_MODULES(X11, [x11], have_x11=yes, have_x11=no)

  if test "x$have_x11" != "xyes"; then
    # Fallback for X11 releases without .pc-files
    AC_PATH_XTRA
    if test x$no_x = xyes ; then
      AC_MSG_ERROR([X development libraries (libX11) not found])
    else
      X11_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
      AC_SUBST(X11_LIBS)
      have_x11=yes
    fi
  fi
fi

# *********
# GSettings
# *********

GLIB_GSETTINGS

# ***************
# Installed tests
# ***************
BEHAVE_INSTALLED_TESTS

AC_CONFIG_FILES([
Makefile
src/Makefile
jpegutils/Makefile
cut-n-paste/Makefile
cut-n-paste/toolbar-editor/Makefile
help/Makefile
po/Makefile.in
data/Makefile
data/xviewer.pc
data/xviewer.desktop.in
data/org.x.viewer.gschema.xml
data/pixmaps/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
doc/Makefile
doc/reference/Makefile
doc/reference/version.xml
doc/reference/xviewer-docs.xml
])

AC_OUTPUT

echo "
Configure summary:

	Source code location .......:  ${srcdir}
	Compiler ...................:  ${CC}
	Extra Compiler Warnings ....:  ${WARN_CFLAGS}

	EXIF support ...............:  ${have_exif}
	XMP support ................:  ${have_exempi}
	JPEG support ...............:  ${have_jpeg}
	Colour management support ..:  ${have_lcms}
	GObject Introspection.......:  ${have_introspection}
"
