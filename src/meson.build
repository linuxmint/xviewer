xviewer_sources = [
    'main.c',
]

private_headers = [
    'xviewer-application-internal.h',
    'xviewer-close-confirmation-dialog.h',
    'xviewer-config-keys.h',
    'xviewer-error-message-area.h',
    'xviewer-image-jpeg.h',
    'xviewer-image-private.h',
    'xviewer-metadata-sidebar.h',
    'xviewer-metadata-reader.h',
    'xviewer-metadata-reader-jpg.h',
    'xviewer-metadata-reader-png.h',
    'xviewer-pixbuf-util.h',
    'xviewer-plugin-engine.h',
    'xviewer-preferences-dialog.h',
    'xviewer-print.h',
    'xviewer-print-image-setup.h',
    'xviewer-print-preview.h',
    'xviewer-save-as-dialog-helper.h',
    'xviewer-session.h',
    'xviewer-util.h',
    'zoom.h',
]

public_headers = [
    'xviewer-application.h',
    'xviewer-application-activatable.h',
    'xviewer-clipboard-handler.h',
    'xviewer-debug.h',
    'xviewer-enums.h',
    'xviewer-file-chooser.h',
    'xviewer-image.h',
    'xviewer-image-save-info.h',
    'xviewer-job-scheduler.h',
    'xviewer-jobs.h',
    'xviewer-list-store.h',
    'xviewer-properties-dialog.h',
    'xviewer-scroll-view.h',
    'xviewer-sidebar.h',
    'xviewer-statusbar.h',
    'xviewer-thumbnail.h',
    'xviewer-thumb-nav.h',
    'xviewer-thumb-view.h',
    'xviewer-transform.h',
    'xviewer-uri-converter.h',
    'xviewer-window-activatable.h',
    'xviewer-window.h',
]

libxviewer_sources = [
    'xviewer-application-activatable.c',
    'xviewer-application.c',
    'xviewer-clipboard-handler.c',
    'xviewer-close-confirmation-dialog.c',
    'xviewer-debug.c',
    'xviewer-error-message-area.c',
    'xviewer-file-chooser.c',
    'xviewer-image.c',
    'xviewer-image-jpeg.c',
    'xviewer-image-save-info.c',
    'xviewer-jobs.c',
    'xviewer-job-scheduler.c',
    'xviewer-list-store.c',
    'xviewer-metadata-details.c',
    'xviewer-metadata-reader.c',
    'xviewer-metadata-reader-jpg.c',
    'xviewer-metadata-reader-png.c',
    'xviewer-metadata-sidebar.c',
    'xviewer-pixbuf-util.c',
    'xviewer-plugin-engine.c',
    'xviewer-preferences-dialog.c',
    'xviewer-print.c',
    'xviewer-print-image-setup.c',
    'xviewer-print-preview.c',
    'xviewer-properties-dialog.c',
    'xviewer-save-as-dialog-helper.c',
    'xviewer-scroll-view.c',
    'xviewer-session.c',
    'xviewer-sidebar.c',
    'xviewer-statusbar.c',
    'xviewer-thumbnail.c',
    'xviewer-thumb-nav.c',
    'xviewer-thumb-view.c',
    'xviewer-transform.c',
    'xviewer-uri-converter.c',
    'xviewer-util.c',
    'xviewer-window-activatable.c',
    'xviewer-window.c',
    'zoom.c'
]

# enums as .c and .h
xviewer_enums = gnome.mkenums(
    'xviewer-enum-types',
    sources: public_headers,
    c_template: 'xviewer-enum-types.c.template',
    h_template: 'xviewer-enum-types.h.template',
    identifier_prefix: 'Xviewer',
    symbol_prefix: 'xviewer'
)

# enums as .xml for gschemas
mkenums = gnome.mkenums(
    'org.x.viewer.enums.xml',
    sources: ['xviewer-scroll-view.h', 'xviewer-window.h'],
    comments: '<!-- @comment@ -->',
    fhead: '<schemalist>',
    vhead: '  <@type@ id=\'org.x.viewer.@EnumName@\'>',
    vprod: '    <value nick=\'@valuenick@\' value=\'@valuenum@\'/>',
    vtail: '  </@type@>',
    ftail: '</schemalist>',
    install_header: true,
    install_dir: join_paths(datadir, 'glib-2.0', 'schemas'),
)

xviewer_marshal = gnome.genmarshal(
    'xviewer-marshal',
    sources: 'xviewer-marshal.list',
    prefix: 'xviewer_marshal'
)

xviewer_resources = gnome.compile_resources(
    'xviewer-resources',
    'resources/xviewer.gresource.xml',
    source_dir: 'resources',
    c_name: '_xviewer'
)

xviewer_deps = [
    config_h,
    gio_unix,
    glib,
    gtk,
    libpeas,
    libpeas_gtk,
    math,
    X11,
    zlib,
    cinnamon_desktop,
    xapp,
    exif,
    exempi,
    lcms,
    rsvg
]

if exif.found()
    libxviewer_sources += 'xviewer-exif-util.c'
    public_headers += 'xviewer-exif-util.h'
endif

if jpeg.found()
    xviewer_deps += jpegutils_dep
endif

libxviewer = library(
    meson.project_name(),
    libxviewer_sources + xviewer_enums + xviewer_marshal + xviewer_resources,
    dependencies: xviewer_deps,
    include_directories: include_dirs,
    install: true,
    install_dir: xviewer_pkglibdir,
    install_rpath: xviewer_pkglibdir
)

install_headers(
    public_headers,
    subdir: meson.project_name()
)

libxviewer_dep = declare_dependency(
    dependencies: xviewer_deps,
    link_with: libxviewer,
    include_directories: include_dirs
)

xviewer = executable(
    meson.project_name(),
    xviewer_sources,
    dependencies: libxviewer_dep,
    install: true,
    install_rpath: xviewer_pkglibdir,
)

gnome.generate_gir(
    libxviewer,
    namespace: 'Xviewer',
    nsversion: '3.0',
    sources: public_headers + libxviewer_sources + xviewer_enums + xviewer_marshal,
    includes: ['Gtk-3.0'],
    identifier_prefix: 'Xviewer',
    symbol_prefix: meson.project_name(),
    install: true,
    install_dir_gir: xviewer_pkgdatadir / 'gir-1.0',
    install_dir_typelib: xviewer_pkglibdir / 'girepository-1.0',
)

pkgconfig.generate(
    filebase: meson.project_name(),
    name: 'Xviewer',
    description: 'An Image Viewer',
    libraries: libxviewer,
    version: version,
    variables: [
        'exec_prefix=${prefix}',
        'pluginsdir=${libdir}/xviewer/plugins'
    ]
)
