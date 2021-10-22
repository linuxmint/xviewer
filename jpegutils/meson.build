# adopted from eog:
# https://github.com/GNOME/eog/blob/master/jpegutils/meson.build

if have_jpeg_80
    jpegutils_sources = 'transupp-8a.c'
else
    jpegutils_sources = 'transupp-6b.c'
endif

libxviewer_jpegutils = static_library(
    'xviewer-jpegutils',
    jpegutils_sources,
    dependencies: [jpeg, config_h],
    include_directories: include_directories('.'),
    install: false
)

jpegutils_dep = declare_dependency(
    dependencies: jpeg,
    include_directories: include_directories('.'),
    link_with: libxviewer_jpegutils,
)
