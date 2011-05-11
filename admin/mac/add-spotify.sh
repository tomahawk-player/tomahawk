#!/bin/sh
# author: lfranchi@kde.org
# usage:  Run from inside the bundle root directory, eg. Tomahawk.app
#         The first parameter should be the spotify resolver binary to copy.
# eg:     add-spotify.sh /path/to/spotify_tomahawkresolver
################################################################################

mkdir -p Contents/Frameworks
cp -R /Library/Frameworks/libspotify.framework Contents/Frameworks

install_name_tool -change /usr/local/Cellar/qt/4.7.3/lib/QtCore.framework/Versions/4/QtCore  @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $1
install_name_tool -change /usr/local/Cellar/qt/4.7.3/lib/QtNetwork.framework/Versions/4/QtNetwork  @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $1
install_name_tool -change libqjson.0.dylib  @executable_path/libqjson.0.7.1.dylib $1
install_name_tool -change /usr/local/Cellar/qjson/0.7.1/lib/libqjson.0.7.1.dylib @executable_path/libqjson.0.7.1.dylib $1
mkdir -p Contents/MacOS
cp $1 Contents/MacOS/
