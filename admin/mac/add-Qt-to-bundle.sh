#!/bin/sh
# author: max@last.fm
# usage:  Run from inside the bundle root directory, eg. Last.fm.app
#         The first parameter should be the QtFrameworks to copy.
#         Remaining parameters are plugins to copy, directories and files are 
#         valid.
# eg:     add-Qt-to-bundle.sh 'QtCore QtGui QtXml' \
#                             imageformats \
#                             sqldrivers/libsqlite.dylib
################################################################################

QT_FRAMEWORKS_DIR="$QTDIR/lib"
QT_PLUGINS_DIR="$QTDIR/plugins"

if [[ ! -d "$QTDIR/lib/QtCore.framework" ]]
then
    # this dir is the location of install for the official Trolltech dmg 
    if [[ -d /Library/Frameworks/QtCore.framework ]]
    then
        QT_FRAMEWORKS_DIR=/Library/Frameworks
        QT_PLUGINS_DIR=/Developer/Applications/Qt/plugins
    fi
fi

echo "Plugins go to: $QT_PLUGINS_DIR"

if [ -z $QTDIR ]
then
    echo QTDIR must be set, or install the official Qt dmg
    exit 1
fi
################################################################################


#first frameworks
mkdir -p Contents/Frameworks
for x in $1
do
    echo "C $x"
    cp -R $QT_FRAMEWORKS_DIR/$x.framework Contents/Frameworks/
    chmod -R u+rw Contents/Frameworks/
done

#plugins
shift
mkdir -p Contents/MacOS
mkdir -p Contents/MacOS/sqldrivers
mkdir -p Contents/MacOS/imageformats
mkdir -p Contents/MacOS/phonon_backend
mkdir -p Contents/MacOS/crypto

cp -R $QT_PLUGINS_DIR/sqldrivers/libqsqlite.dylib Contents/MacOS/sqldrivers/
cp -R $QT_PLUGINS_DIR/imageformats/libqgif.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/imageformats/libqjpeg.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/imageformats/libqico.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/imageformats/libqmng.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/imageformats/libqsvg.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/imageformats/libqtiff.dylib Contents/MacOS/imageformats/
cp -R $QT_PLUGINS_DIR/crypto/libqca-ossl.dylib Contents/MacOS/crypto/
cp -R $QT_PLUGINS_DIR/phonon_backend/phonon_vlc.so Contents/MacOS/phonon_backend/

#cleanup
find Contents/Frameworks -name Headers -o -name \*.prl -o -name \*_debug | xargs rm -rf
find Contents -name \*_debug -o -name \*_debug.dylib | xargs rm
