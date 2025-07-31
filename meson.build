project('xviewer', 'c', version : '3.4.10', meson_version : '>= 0.49')

version = meson.project_version()

xviewer_conf = configuration_data()
xviewer_conf.set_quoted('VERSION', version)
xviewer_conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
xviewer_conf.set('HAVE_INTROSPECTION', 1)

# directories
prefix = get_option('prefix')
datadir = prefix / get_option('datadir')
libdir = prefix / get_option('libdir')
localedir = prefix /  get_option('localedir')

xviewer_pkgdatadir = datadir / meson.project_name()
xviewer_pkglibdir = libdir / meson.project_name()
# workaround for plugins: keep old lib dir without arch-triplet
xviewer_pluginsdir = prefix / 'lib' /  meson.project_name() / 'plugins'

xviewer_conf.set_quoted('LIBDIR', libdir)
xviewer_conf.set_quoted('XVIEWER_DATA_DIR', xviewer_pkgdatadir)
xviewer_conf.set_quoted('XVIEWER_LOCALE_DIR', localedir)
xviewer_conf.set_quoted('XVIEWER_PLUGIN_DIR', xviewer_pluginsdir)

src_root = meson.source_root()
po_dir = join_paths(src_root, 'po')

# options
enable_deprecation_warnings = get_option('deprecated_warnings')
enable_docs = get_option('docs')

gnome = import('gnome')
i18n = import('i18n')
pkgconfig = import('pkgconfig')

# dependencies
gio_unix = dependency('gio-unix-2.0', version: '>= 2.38.0')
glib = dependency('glib-2.0', version: '>= 2.38.0')
gtk = dependency('gtk+-3.0')
libpeas = dependency('libpeas-1.0', version: '>= 0.7.4')
libpeas_gtk = dependency('libpeas-gtk-1.0', version: '>= 0.7.4')
pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.19.1')
X11 = dependency('x11')
zlib = dependency('zlib')
cinnamon_desktop = dependency('cinnamon-desktop', version: '>= 3.2.0')
xapp = dependency('xapp', version: '>= 2.5.0')

cc = meson.get_compiler('c')

# on some systems we need to find the math lib to make sure it builds
math = cc.find_library('m', required: false)

# taken from eog's meson.build
# support for nl_langinfo (_NL_MEASUREMENT_MEASUREMENT) (optional)
langinfo_measurement_src = '''
  #include <langinfo.h>
  int main() {
    char c;
    c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
  };
'''
xviewer_conf.set('HAVE__NL_MEASUREMENT_MEASUREMENT', cc.compiles(langinfo_measurement_src),
             description: 'Define if _NL_MEASUREMENT_MEASUREMENT is available')

# support for strptime
xviewer_conf.set('HAVE_STRPTIME', cc.has_function('strptime'))

# (optional) XMP support
exempi = dependency('exempi-2.0', version: '>= 1.99.5', required: get_option('exempi'))
if exempi.found()
    xviewer_conf.set10('HAVE_EXEMPI', true)
endif

# (optional) EXIF support
exif = dependency('libexif', version: '>= 0.6.14', required: get_option('exif'))
if exif.found()
    xviewer_conf.set10('HAVE_EXIF', true)
endif

# (semi-optional) JPEG support
jpeg = dependency('libjpeg', required: get_option('jpeg'))
if jpeg.found()
    xviewer_conf.set10('HAVE_JPEG', true)
    xviewer_conf.set10('HAVE_LIBJPEG', true)

    jpeg_80_check_src = '''
      #include <stdio.h>
      #include <jpeglib.h>
      #if JPEG_LIB_VERSION < 80
      #error "wrong version"
      #endif
    '''
    have_jpeg_80 = cc.compiles(jpeg_80_check_src, dependencies: jpeg, name: 'libjpeg version is 8 or greater')
endif

lcms = dependency('lcms2', required: get_option('lcms'))
if lcms.found()
    xviewer_conf.set10('HAVE_LCMS', true)
endif

rsvg = dependency('librsvg-2.0', version: '>= 2.36.2', required: get_option('rsvg'))
if rsvg.found()
    xviewer_conf.set10('HAVE_RSVG', true)
endif

# generate config.h
config_h_file = configure_file(
    output : 'config.h',
    configuration : xviewer_conf
)

config_h = declare_dependency(
    sources: config_h_file,
    include_directories: include_directories('.')
)

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

if not enable_deprecation_warnings
    add_global_arguments([
        '-Wno-deprecated-declarations',
        '-Wno-deprecated',
        '-Wno-declaration-after-statement'
        ],
        language: 'c'
    )
endif

include_dirs = [include_directories('src')]

subdir('data')
subdir('install-scripts')

if jpeg.found()
    subdir('jpegutils')

    include_dirs += include_directories('jpegutils')
    jpegutils_dep = get_variable('jpegutils_dep')
endif

subdir('src')
subdir('help')
subdir('po')

if enable_docs
    subdir('doc/reference')
endif

summary_msg = '---- Summary ----\n'
summary_msg += 'Paths:\n'
summary_msg += '  prefix             : ' + prefix + '\n'
summary_msg += '  data dir           : ' + datadir + '\n'
summary_msg += '  lib dir            : ' + libdir + '\n'
summary_msg += '  locale dir         : ' + localedir + '\n'
summary_msg += '  plugins dir        : ' + xviewer_pluginsdir + '\n'
summary_msg += 'Configuration:\n'
summary_msg += '  deprecated_warnings: ' + enable_deprecation_warnings.to_string() + '\n'
summary_msg += '  docs               : ' + enable_docs.to_string() + '\n'
summary_msg += 'Features:\n'
summary_msg += '  exempi             : ' + exempi.found().to_string() + '\n'
summary_msg += '  exif               : ' + exif.found().to_string() + '\n'
summary_msg += '  jpeg               : ' + jpeg.found().to_string() + '\n'
summary_msg += '  lcms               : ' + lcms.found().to_string() + '\n'
summary_msg += '  rsvg               : ' + rsvg.found().to_string() + '\n'

message(summary_msg)
