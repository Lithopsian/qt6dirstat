# QDirStat
<img src="https://github.com/Lithopsian/qt6dirstat/blob/main/src/icons/qdirstat.svg" height="64">

Qt-based directory statistics

(c) 2015-2023 Stefan Hundhammer <Stefan.Hundhammer@gmx.de> and 2023-2024 Ian Nartowicz

Target Platforms: Linux, BSD, Unix-like systems

License: GPL V2

Updated: 2024-04-03


## Screenshot

[<img width="900" src="https://github.com/Lithopsian/qt6dirstat/blob/main/screenshots/QDirStat-main-win.png">](https://github.com/Lithopsian/qt6dirstat/blob/main/screenshots/QDirStat-main-win.png)

_Main window screenshot - notice the multi-selection in the tree and the treemap_


## Overview

This project is a fork of QDirStat ( see https://github.com/shundhammer/qdirstat for the original).
It was initially to finish the Treemap work that I started at QDirStat, then to port to
Qt6, and then a lot more.  It corresponds approximately to QDirStat 1.9, but without bookmarks.  It
is essentially backwards (and so far forwards) compatible with QDirStat, although the configuration
settings are in a different directory, so you can run both at once and independently, but would have
to copy the old configuration files to keep settings from QDirStat.

The main changes:
- the Treemap is better-looking, more-easily configurable, multi-threaded to reduce contention
with the rest of the application, and overall a lot faster;
- the Mime categorizer is faster and follows better precedence rules for files which match
multiple patterns;
- the program is ported to Qt6, while still being conpatible with Qt5, but not Qt4;
- symbolic links are now supported for dpkg packages, essential for many Debian and Ubuntu
installations with merged /usr directories, and the fairly uncommon diversions are also
supported;
- a number of crashes and freezes have been fixed, as well as lot of less serious problems;
- there is an extra tab in the settings dialog for the Treemap, and the settings on the general
tab now take effect without having to restart the program;
- the cache mechanism

- Package manager support:
  - Show what software package a system file belongs to.
  - [Packages view](doc/Pkg-View.md) showing disk usage of installed software
    packages and their individual files.
  - [Unpackaged files view](doc/Unpkg-View.md) showing what files in system
    directories do not belong to any installed software package.

- New views:
  - Disk usage per file type (by filename extension).
  - File size histogram view.
  - [File Age View](doc/File-Age-Stats.md)
  - Free, used and reserved disk size for each mounted filesystem (like _df_)

See section [_New Features_](#new-features) for more details.


## Table of Contents

1. [Screenshot](#screenshot)
1. [Latest Stable Release](#latest-stable-release)
1. [Latest News](#latest-news)
1. [History](#history)
1. [Related Software](#related-software): KDirStat, WinDirStat, K4DirStat and more
1. [Motivation / Rant: Why?](#motivation--rant-why)
1. [Features](#features)
1. [MacOS X Compatibility](#macos-x-compatibility)
1. [Windows Compatibility](#windows-compatibility)
1. [Ready-made Packages](#ready-made-packages)
1. [QDirStat Docker Containers](#qdirstat-docker-containers)
1. [Building](#building)
1. [Contributing](#contributing)
1. [Troubleshooting](#troubleshooting)
1. [Further Reading](#further-reading)
1. [Packaging Status](#packaging-status)
1. [Donate](#donate)


-----------------------

## Donate

QDirStat is Free Open Source Software.

If you find it useful, please consider donating.
You can donate any amount of your choice via PayPal:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=EYJXAVLGNRR5W)



## History


See [DevHistory.md](doc/DevHistory.md) for history of QDirStat.




## Related Software

### KDirStat and QDirStat

KDirStat was the first program of this kind (combining a traditional tree view
with a treemap), also written by the same author as QDirStat. It was made for
KDE 1 back in early 2000; later ported to KDE 2, then KDE 3.

QDirStat is based on that code, but made independent of any KDE libraries or
infrastructure, so it has much fewer library and package dependencies;
basically only the Qt 5 libs and libz, both of which most Linux / BSD machines
have installed anyway if there is any graphical desktop installed.


### WinDirStat and QDirStat

There are lots of articles and user forum comments about QDirStat being a "nice
Linux port of WinDirStat". Well, nothing could be further from the truth:
**WinDirStat is a Windows port of KDirStat**, the predecessor of QDirStat.

So it's the other way round: **The Linux version was there first**, and
somebody liked it so much that he wrote a Windows version based on that
idea. That's a rare thing; usually people port Windows originals to Linux.

See also https://windirstat.net/background.html and the WinDirStat "About"
dialog.



### Other

- Baobab
- Filelight
- ncdu
- du

See
[Disk Usage Tools Compared](https://github.com/shundhammer/qdirstat/wiki/disk-usage-tools-compared):
QDirStat vs. K4DirStat vs. Baobab vs. Filelight vs. ncdu (including benchmarks)
in the Wiki.



## Building

### Build Environment

Make sure you have a working Qt5 or Qt6 build environment installed. This includes:

- C++ compiler (gcc recommended)
- Qt runtime environment
- Qt header files
- libz (compression lib) runtime and header file

If anything doesn't work, first of all **make sure you can build any of the
simple examples supplied with Qt**, e.g. the
[calculator example](http://doc.qt.io/qt-5/qtwidgets-widgets-calculator-example.html).

You will also need a C++11-compatible compiler, but that is almost all of them.

#### Ubuntu

Install the required packages for building:

    sudo apt-get install build-essential qtbase5-dev zlib1g-dev

Dependent packages will be added automatically.

Recommended packages for developers:

    sudo apt-get install qttools5-dev-tools qtbase5-doc qtbase5-doc-html qtbase5-examples

See also

https://askubuntu.com/questions/508503/whats-the-development-package-for-qt5-in-14-04

If you have both Qt5 and Qt6, you can use qtchooser or manually set the QT_SELECT
environment variable to build against the correct version.


#### SUSE

Install the required packages for building:

    sudo zypper install -t pattern devel_C_C++
    sudo zypper install libQt5Widgets-devel libqt5-qttools zlib-devel

If you have both Qt5 and Qt6 development environments installed, make sure that the
correct version of 'qmake' is the first in your $PATH.


### Compiling

Open a shell window, go to the QDirStat source directory, then enter these
commands:

    qmake
    make


### Installing

    sudo make install

or

    su -c make install


### Install to a Custom Directory

The default setup installs everything to `/usr`. To install to another
directory, set `INSTALL_PREFIX` during `qmake`.

<details>

    qmake INSTALL_PREFIX=/usr/local

Beware that some things might not work as expected; for example, you will not
get a `.desktop` file in the proper place to make QDirStat appear in any menus
in your graphical desktop environment or in the file manager. You will need to
copy the `.desktop` file manually to whatever directory your graphical desktop
environment uses somewhere in your home directory. Similar with the application
icon used in that `.desktop` file.
</details>


## Contributing

See file [Contributing.md](doc/Contributing.md)
and [GitHub-Workflow.md](doc/GitHub-Workflow.md)


## Troubleshooting

### Can't Move a Directory to Trash

See file [Troubleshooting.md](doc/Troubleshooting.md)


## Further Reading

- Original [KDirStat](http://kdirstat.sourceforge.net/)  ([source code](https://github.com/shundhammer/kdirstat))
- [K4Dirstat](https://bitbucket.org/jeromerobert/k4dirstat/wiki/Home) ([source code](https://bitbucket.org/jeromerobert/k4dirstat/src))
- [WinDirStat](https://windirstat.info/) (for Windows)
- [Disk Usage Tools Compared: QDirStat vs. K4DirStat vs. Baobab vs. Filelight vs. ncdu](https://github.com/shundhammer/qdirstat/wiki/disk-usage-tools-compared) (including benchmarks)
- [XDG Trash Spec](http://standards.freedesktop.org/trash-spec/trashspec-1.0.html)
- [Spatry's QDirStat Review on YouTube](https://www.youtube.com/watch?v=ysm4-x_5ftI)

Of course, don't forget to check out the [doc directory](doc/).


## Packaging Status

Repology: QDirStat versions in Linux / BSD distributions:

[![Repology](https://repology.org/badge/tiny-repos/qdirstat.svg)](https://repology.org/metapackage/qdirstat/versions)

(click for details)


## Donate

QDirStat is Free Open Source Software.

If you find it useful, please consider donating.
You can donate any amount of your choice via PayPal:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=EYJXAVLGNRR5W)
