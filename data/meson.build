custom_target(
    'appdata',
    input: 'xviewer.appdata.xml.in',
    output: 'xviewer.appdata.xml',
    command: [intltool_merge, '-x', '-u', po_dir, '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: join_paths(datadir, 'metainfo'),
    build_by_default: true
)

custom_target(
    'desktop',
    input: 'xviewer.desktop.in.in',
    output: 'xviewer.desktop',
    command: [intltool_merge, '-d', '-u', po_dir, '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: join_paths(datadir, 'applications'),
    build_by_default: true
)

gschema_conf = configuration_data()
gschema_conf.set('GETTEXT_PACKAGE', meson.project_name())

configure_file(
    input: 'org.x.viewer.gschema.xml.in',
    output: 'org.x.viewer.gschema.xml',
    configuration: gschema_conf,
    install_dir: join_paths(datadir, 'glib-2.0', 'schemas')
)

subdir('icons')

install_subdir(
    'pixmaps',
    install_dir: join_paths(datadir, meson.project_name())
)