Tomahawk
========

Tomahawk is a free multi-source and cross-platform music player. An application that can play not only your local files, but also stream from services like Spotify, Beats, SoundCloud, Google Music, YouTube and many others. You can even connect with your friends' Tomahawks, share your musical gems or listen along to them. Let the music play!

![Tomahawk Screenshot](/data/screenshots/tomahawk-screenshot.png?raw=true)


Downloading our latest nightly builds
-------------------------------------
* OS X: http://download.tomahawk-player.org/nightly/mac/Tomahawk-latest.dmg
* Windows: http://download.tomahawk-player.org/nightly/windows/tomahawk-latest.exe

Downloading our latest stable builds
------------------------------------
* Linux: http://www.tomahawk-player.org/download.html#linux
* OS X: http://download.tomahawk-player.org/Tomahawk-0.7.0.dmg
* Windows: http://download.tomahawk-player.org/tomahawk-0.7.0.exe

Compiling and running Tomahawk
------------------------------

Compile:

    $ mkdir build && cd build
    $ cmake ..
    $ make

Start the application on Linux:

    $ ./tomahawk

Detailed build instructions
---------------------------
* Arch: http://wiki.tomahawk-player.org/index.php/Building_ArchLinux_package
* Debian: http://wiki.tomahawk-player.org/index.php/Building_on_Debian
* Fedora: http://wiki.tomahawk-player.org/index.php/Building_on_Fedora
* openSUSE: http://wiki.tomahawk-player.org/index.php/Building_on_openSUSE
* Ubuntu: http://wiki.tomahawk-player.org/index.php/Building_on_Ubuntu
* OS X: http://wiki.tomahawk-player.org/index.php/Building_OS_X_Application_Bundle
* Windows: http://wiki.tomahawk-player.org/index.php/Building_Windows_Binary

Doxygen Documentation
---------------------
See: http://dev.tomahawk-player.org/api/classes.html

Dependencies
------------

Required dependencies:

* CMake 2.8.6 - http://www.cmake.org/
* Qt 4.7.0 - http://qt-project.org/
* Phonon 4.6.0 - http://phonon.kde.org/
* QJson 0.8.1 - http://qjson.sourceforge.net/
* SQLite 3.6.22 - http://www.sqlite.org/
* TagLib 1.8 - http://developer.kde.org/~wheeler/taglib.html
* Boost 1.3 - http://www.boost.org/
* Lucene++ 3.0.6 - https://github.com/luceneplusplus/LucenePlusPlus/
* libechonest 2.2.0 - http://projects.kde.org/projects/playground/libs/libechonest/
* Attica 0.4.0 - ftp://ftp.kde.org/pub/kde/stable/attica/
* QuaZip 0.4.3 - http://quazip.sourceforge.net/
* liblastfm 1.0.1 - https://github.com/lastfm/liblastfm/
* QtKeychain 0.1 - https://github.com/frankosterfeld/qtkeychain/
* Sparsehash - https://code.google.com/p/sparsehash/
* GnuTLS - http://gnutls.org/

The following dependencies are optional, but recommended:

* Jreen 1.0.5 (1.1.0 will fail, 1.1.1 is fine) - http://qutim.org/jreen/
* Snorenotify - https://github.com/Snorenotify/Snorenotify/

Third party libraries that we ship with our source:

* MiniUPnP 1.6 - http://miniupnp.free.fr/
* Qocoa - https://github.com/mikemcquaid/Qocoa/
* libqnetwm - http://code.google.com/p/libqnetwm/
* libqxt (QxtWeb module) - http://libqxt.org/
* SPMediaKeyTap - https://github.com/nevyn/SPMediaKeyTap/
* kdSingleApplicationGuard - http://www.kdab.com/

Enjoy!
