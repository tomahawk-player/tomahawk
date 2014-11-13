# TOMAHAWK

## WHAT TOMAHAWK IS

Tomahawk is a free multi-source and cross-platform music player. An application that can play not only your local files, but also stream from services like Spotify, Beats, SoundCloud, Google Music, YouTube and many others. You can even connect with your friends' Tomahawks, share your musical gems or listen along to them. Let the music play!

![Tomahawk Screenshot](/data/screenshots/tomahawk-screenshot.png?raw=true)

## HOW TOMAHAWK WORKS

Tomahawk is basically a **music metadata player**.  At its core it decouples the metadata about a song from the source and reassembles it for each user based on their individual music accessibility and rights. In short, given the name of a song and artist Tomahawk will find the right source, for the right user at the right time.  This fundamentally different approach to music enables a range of new music consumption and sharing experiences previously not possible. 

## MUSIC SOURCES

* Local music library (MP3, Ogg, FLAC and many other formats)
* Networked music libraries (other connected computers)

### Subscription Music Services

* Spotify
* Beats Music
* Google Play Music (on-demand streaming and music locker)
* Grooveshark 
* Rdio (Android only)
* Deezer (Android only)

### Free Streaming/Music Promotion Platforms

* Soundcloud
* Bandcamp
* Last.fm
* Jamendo
* Official.fm

### Network/Cloud storage

* Ampache
* Owncloud
* Subsonic
* Beets

Third party-developed resolvers have also been written for services like YouTube, Qobuz and others. We've also heard of digital music distributors writing their own for their internal CMSes to help them navigate and preview their content. That's cool.

Packaged binary resolvers (.axes) are available: [here](http://teom.org/axes).

Source code (and examples) can be found in our [Resolver repository](https://github.com/tomahawk-player/tomahawk-resolvers).

## DOWNLOAD TOMAHAWK

You can download one of our nightly or stable builds:

| *BUILD* | MAC / OSX | WINDOWS | LINUX |
|:-------:|:---------:|:-------:|:-----:|
|**NIGHTLY** | [**latest**](http://download.tomahawk-player.org/nightly/mac/Tomahawk-latest.dmg) | [**latest**](http://download.tomahawk-player.org/nightly/windows/tomahawk-latest.exe) | [**latest**] (https://launchpad.net/~tomahawk/+archive/ubuntu/nightly) (Ubuntu) |
|**STABLE** | [**0.8**](http://download.tomahawk-player.org/Tomahawk-0.8.0.dmg) | [**0.8**](http://download.tomahawk-player.org/tomahawk-0.8.0.exe) | [**0.8**](http://www.tomahawk-player.org/#page-about) (various distros) |

## BUILD TOMAHAWK

... or you can compile it yourself:

    $ mkdir build && cd build
    $ cmake ..
    $ make

### Detailed Build Instructions

**LINUX**

* [Arch](http://wiki.tomahawk-player.org/index.php/Building_ArchLinux_package)
* [Debian](http://wiki.tomahawk-player.org/index.php/Building_on_Debian)
* [Fedora](http://wiki.tomahawk-player.org/index.php/Building_on_Fedora)
* [openSUSE](http://wiki.tomahawk-player.org/index.php/Building_on_openSUSE)
* [Ubuntu](http://wiki.tomahawk-player.org/index.php/Building_on_Ubuntu)

**MAC**

* [OS X](http://wiki.tomahawk-player.org/index.php/Building_OS_X_Application_Bunde)

**WINDOWS**

* [Windows](http://wiki.tomahawk-player.org/index.php/Building_Windows_Binary)

### Dependencies

Required dependencies:

* [CMake 2.8.6](http://www.cmake.org/)
* [Qt 4.7.0](http://qt-project.org/)
* [VLC 2.1.0](https://videolan.org/vlc/)
* [QJson 0.8.1](http://qjson.sourceforge.net/)
* [SQLite 3.6.22](http://www.sqlite.org/)
* [TagLib 1.8](http://developer.kde.org/~wheeler/taglib.html)
* [Boost 1.3](http://www.boost.org/)
* [Lucene++ 3.0.6](https://github.com/luceneplusplus/LucenePlusPlus/)
* [libechonest 2.2.0](http://projects.kde.org/projects/playground/libs/libechonest/)
* [Attica 0.4.0](http://ftp.kde.org/stable/attica/)
* [QuaZip 0.4.3](http://quazip.sourceforge.net/)
* [liblastfm 1.0.1](https://github.com/lastfm/liblastfm/)
* [QtKeychain 0.1](https://github.com/frankosterfeld/qtkeychain/)
* [Sparsehash](https://code.google.com/p/sparsehash/)
* [GnuTLS](http://gnutls.org/)

The following dependencies are optional (but *recommended*):

* [Jreen 1.0.5](http://qutim.org/jreen/) (1.1.0 will fail, 1.1.1 is fine)
* [Snorenotify](https://github.com/Snorenotify/Snorenotify/)

Third party libraries that we ship with our source:

* [MiniUPnP 1.6](http://miniupnp.free.fr/)
* [Qocoa](https://github.com/mikemcquaid/Qocoa/)
* [libqnetwm](https://code.google.com/p/libqnetwm/)
* [libqxt](http://libqxt.org/) (QxtWeb module) 
* [SPMediaKeyTap](https://github.com/nevyn/SPMediaKeyTap/)
* [kdSingleApplicationGuard](http://www.kdab.com/)

## SUPPORT TOMAHAWK

* [Bug Tracker & Issues](https://bugs.tomahawk-player.org/secure/Dashboard.jspa)
* [Translations](https://www.transifex.com/projects/p/tomahawk/)
* [Donate](https://flattr.com/thing/169312/Tomahawk)

## GET HELP

* [Support & Feedback](https://tomahawk.uservoice.com)
* Chat with Us (IRC): #tomahawk (on Freenode)
* [Twitter](https://twitter.com/tomahawk)
* [Facebook](https://facebook.com/tomahawkplayer)
* [Developer API documentation](http://dev.tomahawk-player.org/api/classes.html)

## SCREENSHOTS

BROWSE FRIENDS' MUSIC & LISTEN ALONG

![Browse](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410998050088_listen-along.jpg)

INBOX - RECEIVED & FORWARDING

![Inbox](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410997751044_inbox.jpg)

CHARTS - BILLBOARD'S TASTEMAKER ALBUMS

![Charts](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410997901969_charts.jpg)

FRIEND FEED

![Feed](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410971283885_heroshot.png)

DYNAMIC (AUTO-UPDATING) PLAYLIST

![Xspf](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410998362549_dynamic-playlist-1.jpg)

PLUG-INS / RESOLVER SETTINGS

![Settings](https://dchtm6r471mui.cloudfront.net/hackpad.com_ZRZMJDdxrVe_p.242147_1410998587408_prefs.jpg)

**Enjoy!**
