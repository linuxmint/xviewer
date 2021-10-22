docs_ignore_h = [
    'xviewer-application-service.h',
    'xviewer-close-confirmation-dialog.h',
    'xviewer-enum-types.h',
    'xviewer-image-private.h',
    'xviewer-image-jpeg.h',
    'xviewer-marshal.h',
    'xviewer-metadata-details.h',
    'xviewer-metadata-reader.h',
    'xviewer-metadata-reader-jpg.h',
    'xviewer-metadata-reader-png.h',
    'xviewer-metadata-sidebar.h',
    'xviewer-module.h',
    'xviewer-plugin-engine.h',
    'xviewer-preferences-dialog.h',
    'xviewer-print-image-setup.h',
    'xviewer-print-preview.h',
]

if not exif.found()
if not exempi.found()
    docs_ignore_h += 'xviewer-exif-util.h'
endif
endif

gnome.gtkdoc(
    meson.project_name(),
    mode: 'xml',
    install: true,
    main_xml: 'xviewer-docs.xml',
    src_dir: [
        join_paths(meson.source_root(), 'src'),
        join_paths(meson.build_root(), 'src'),
    ],
    ignore_headers: docs_ignore_h,
    dependencies: libxviewer_dep
)