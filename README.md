Tomahawk
========

Compiling and running Tomahawk
------------------------------

Compile:

    $ mkdir build && cd build
    $ cmake ..
    $ make

Start the application on Linux:

    $ ./tomahawk

Start the application on OS X:

    $ open tomahawk.app


Detailed building instructions for Ubuntu
-----------------------------------------
See: http://wiki.tomahawk-player.org/index.php/Building_Binary_on_Ubuntu_Precise_(12.04)

Detailed building instructions for OS X
---------------------------------------
See: http://wiki.tomahawk-player.org/index.php/Building_OS_X_Application_Bundle

Doxygen Documentation
---------------------
See: http://dev.tomahawk-player.org/api/classes.html

Dependencies
------------

Required dependencies:

* CMake 2.8.6 - http://www.cmake.org/
* Qt 4.7.0 - http://qt-project.org/
* Phonon 4.6.0 - http://phonon.kde.org/
* QJson 0.7.1 - http://qjson.sourceforge.net/
* SQLite 3.6.22 - http://www.sqlite.org/
* TagLib 1.6.2 - http://developer.kde.org/~wheeler/taglib.html
* Boost 1.3 - http://www.boost.org/
* CLucene 0.9.23 (0.9.21 will fail) - http://clucene.sourceforge.net/download.shtml
* libechonest 2.0.0 - http://projects.kde.org/projects/playground/libs/libechonest/
* Attica 0.4.0 - ftp://ftp.kde.org/pub/kde/stable/attica/
* QuaZip 0.4.3 - http://quazip.sourceforge.net/
* liblastfm 1.0.1 - https://github.com/lastfm/liblastfm/

The following dependencies are optional, but recommended:

* Jreen 1.0.5 (1.1.0 will fail, 1.1.1 is fine) - http://qutim.org/jreen/
* QTweetLib 0.5.0 - https://github.com/minimoog/QTweetLib/

Third party libraries that we ship with our source:

* MiniUPnP 1.6 - http://miniupnp.free.fr/
* Breakpad - http://code.google.com/p/google-breakpad/
* Qocoa - https://github.com/mikemcquaid/Qocoa/
* libqnetwm - http://code.google.com/p/libqnetwm/
* libqxt (QxtWeb module) - http://libqxt.org/
* SPMediaKeyTap - https://github.com/nevyn/SPMediaKeyTap/
* kdSingleApplicationGuard - http://www.kdab.com/

Enjoy!
