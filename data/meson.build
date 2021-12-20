i18n.merge_file(
    input: 'xviewer.appdata.xml.in',
    output: 'xviewer.appdata.xml',
    po_dir: po_dir,
    type: 'xml',
    install: true,
    install_dir: datadir / 'metainfo'
)

i18n.merge_file(
    input: 'xviewer.desktop.in.in',
    output: 'xviewer.desktop',
    po_dir: po_dir,
    type: 'desktop',
    install: true,
    install_dir: datadir / 'applications'
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