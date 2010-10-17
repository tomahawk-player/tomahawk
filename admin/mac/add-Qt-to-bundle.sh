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


if [[ ! -d "$QTDIR/lib/QtCore.framework" ]]
then
    # this dir is the location of install for the official Trolltech dmg 
    if [[ -d /Library/Frameworks/QtCore.framework ]]
    then
        QT_FRAMEWORKS_DIR=/Library/Frameworks
        QT_PLUGINS_DIR=/Developer/Applications/Qt/plugins
    fi
elif [[ $QTDIR ]]
then
    QT_FRAMEWORKS_DIR="$QTDIR/lib"
    QT_PLUGINS_DIR="$QTDIR/plugins"
fi

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
done

#plugins
shift
mkdir -p Contents/MacOS
while (( "$#" ))
do
    echo "C $1"

    if [[ -d $QT_PLUGINS_DIR/$1 ]]
    then
        cp -R $QT_PLUGINS_DIR/$1 Contents/MacOS
    else
        dir=$(basename $(dirname $1))
        mkdir Contents/MacOS/$dir
        cp $QT_PLUGINS_DIR/$1 Contents/MacOS/$dir
    fi
    
    shift
done

#cleanup
find Contents/Frameworks -name Headers -o -name \*.prl -o -name \*_debug | xargs rm -rf
find Contents -name \*_debug -o -name \*_debug.dylib | xargs rm
