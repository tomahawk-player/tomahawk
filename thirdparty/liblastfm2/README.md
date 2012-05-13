liblastfm
=========
liblastfm is a collection of libraries to help you integrate Last.fm services
into your rich desktop software. It is officially supported software developed
by Last.fm staff.

Michael Coffey  http://twitter.com/eartle

Fork it: http://github.com/eartle/liblastfm


Dependencies
============
liblastfm dynamically links to:

* Qt 4.8
  http://qt.nokia.com/
* FFTW 3.2
  Compiled with single precision
  http://www.fftw.org
* Secret Rabbit code (aka libsamplerate)
  http://www.mega-nerd.com/SRC

Additionally, to build you will need Ruby and GNU make (or Microsoft nmake).

Mac OS X
--------
    sudo port selfupdate
    sudo port upgrade installed
    sudo port install libsamplerate fftw-3 qt4-mac-devel

Linux/*NIX
----------
Do something like this:

    sudo apt-get install qt4-qmake pkg-config libsamplerate-dev libfftw3-dev ruby g++ libqt4-dev

Additionally on Linux the configure process requires lsb_release. This is
usually already installed (correct me if I'm wrong).

Please note, we have only tested on Linux, but we think it'll work on all
varieties of UNIX. If it doesn't, report the bug to eartle on GitHub.

Windows
-------
Install Ruby. Install Visual Studio 2008 or higher. Install Qt. Install the
Windows Server 2003 Platform SDK r2:

http://www.microsoft.com/Downloads/details.aspx?FamilyID=484269e2-3b89-47e3-8eb7-1f2be6d7123a

Set up your environment variables so all include paths and tools are
available.

Build and install FFTW and Secret Rabbit Code.

Open a plain Windows shell (Cygwin will work but we don't recommend it), and
see the next section.


Installing liblastfm
====================
    qmake && make && sudo make install

Packaging liblastfm
-------------------
liblastfm builds to two dynamic libraries (liblastfm.so and
liblastfm_fingerprint.so). liblastfm.so links only to Qt, but the
fingerprinting part has additional dependencies. So ideally, you would
distribute two packages.


Using liblastfm
===============
We have copied the API at http://last.fm/api onto C++, so like you find
artist.getInfo there you will find an lastfm::Artist::getInfo function in our
C++ API. lastfm is a namespace, Artist a class and getInfo a function.

Thus the API is quite easy to learn. We suggest installing and checking the
include/lastfm/* directory to find out all capabilities.

The demos directory shows some further basic usage including Audioscrobbling
and getting metadata for music via our fingerprinting technology.

You need an API key from http://last.fm/api to use the webservice API.

Your link line needs to include the following:

    -llastfm -lQtCore -lQtNetwork -lQtXml

Radio
-----
Please set an identifiable UserAgent on your HTTP requests for the actual MP3s,
in extreme cases we'll contact you directly and demand you do so :P

HTTP & Networking
-----------------
You can specify your own QNetworkAccessManager derived class for liblastfm to
use with lastfm::setNetworkAccessManager(). Our default is pretty good
though, auto-determining proxy settings on Windows and OS X for instance.


Using liblastfm_fingerprint
===========================
The liblastfm_fingerprint library does not decode audio files. We anticipate
that Phonon will soon do that work for us. In the meantime, sample *Source
files for MP3, Ogg Vorbis, FLAC, and AAC/MP4 are available in
src/fingerprint/contrib. If you want to fingerprint files or get metadata
suggestions, you either need to add the *Source files to your project, or
implement your own.


Development
===========
Public Headers
--------------
1. Header guards should be prefixed with LASTFM, eg. LASTFM_WS_REPLY_H
2. #includes should be to the system path eg. #include <lastfm/Scrobbler>
3. Don't make a header public unless it is absolutely required!
4. To make the header public edit the headers.files line in the pro file

Private Headers
---------------
1. For consistency and to make it more obvious it is a private header, don't
   prefix the header guard with LASTFM
2. #includes should be the full source tree path, eg.
   #include "../core/UrlBuilder.h"
