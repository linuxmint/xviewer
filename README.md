# Xviewer - An Image Viewer
![build](https://github.com/linuxmint/xviewer/actions/workflows/build.yml/badge.svg)

Xviewer is a simple image viewer which uses the gdk-pixbuf library.
It can deal with large images, and zoom and scroll with constant memory usage.
Its goals are simplicity and standards compliance.

## Installation

Xviewer is installed by default in Linux Mint.
It is available in the Linux Mint repositories, but not in the official Ubuntu or Debian repositories.
However, compiled deb packages can be found on the [GitHub releases page](https://github.com/linuxmint/xviewer/releases).

## Plugins

Xviewer supports plugins.
Linux Mint provides a few plugins that can be installed via the `xviewer-plugins` package.
Plugins can be enabled in Xviewer via `Edit` > `Preferences` in the `Plugins` tab.

Their source code is available in the [xviewer-plugins repository](https://github.com/linuxmint/xviewer-plugins).

## Build and Install from Source

To build Xviewer from source and install it, perform the following steps.

### For Mint/Ubuntu/Debian based Distros

```bash
# install build tools if necessary
sudo apt install build-essential devscripts equivs git meson

# clone this git repository, switch into cloned directory
git clone https://github.com/linuxmint/xviewer.git && cd xviewer

# generate build-dependency package and install it
mk-build-deps -s sudo -i

# build .deb packages
debuild --no-sign

# install packages
sudo debi
```

## For other Distros

The concrete packages to install depend on your distro.
Please note that we can't guarantee that the available library versions are compatible.

1. Install the following build tools
   1. meson
   2. ninja
   3. yelp-tools
   4. gtk-doc-tools (optional for API docs, required for `docs` build option)
2. Install the following libraries with their headers
   * libatk1.0
   * [libcinnamon-desktop](https://github.com/linuxmint/cinnamon-desktop)
   * libgdk-pixbuf2.0
   * libgirepository1.0
   * libglib2.0
   * libgtk-3
   * libpeas
   * [libxapp](https://github.com/linuxmint/xapp)
   * libxml2
   * zlib1g
3. (Optional) Install libraries and their headers for extra features
   * libexempi
   * libexif
   * libjpeg
   * liblcms2
   * librsvg2
4. Clone this git repository and run the following commands in the cloned directory:

```bash
# prepare build, options can be set with -Doption=value
# Please check the output for errors and the effective build options.
meson setup --prefix=/usr/local build

# compile and install
sudo ninja -C build install

# to uninstall:
sudo ninja -C build uninstall
```
