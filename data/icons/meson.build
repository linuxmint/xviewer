# split up icons into public icons (app icons) and private icons (used inside of xviewer)

sizes = [
    '16x16',
    '22x22',
    '24x24',
    '32x32',
    '48x48',
    'scalable'
]

foreach size : sizes
    # public icons  -> e.g. /usr/share/icons
    install_subdir(
        join_paths(size, 'apps'),
        install_dir: join_paths(datadir, 'icons', 'hicolor', size)
    )

    # private icons -> e.g. /usr/share/xviewer/icons
    install_subdir(
        join_paths(size, 'actions'),
        install_dir: join_paths(datadir, meson.project_name(), 'icons', 'hicolor', size)
    )
endforeach


