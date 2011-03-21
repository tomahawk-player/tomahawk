#!/bin/bash
#
# Usage: ./admin/mac/build-release-osx.sh [--no-clean]
#
################################################################################


function header {
    echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}
################################################################################


ROOT=`pwd`

QTDIR=`which qmake`
LINKDIR=`readlink $QTDIR`
QTDIR=`dirname $QTDIR`
QTDIR=$QTDIR/`dirname $LINKDIR`
QTDIR=`dirname $QTDIR`
test -L "$QTDIR" && QTDIR=`readlink $QTDIR`

echo "Goes here: $QTDIR"

export QMAKESPEC='macx-g++'
export QTDIR
export VERSION
export QTVERSION='4.7.2'
################################################################################


CLEAN='1'
BUILD='1'
NOTQUICK='1'
CREATEDMG='1'

    header "Adding Qt to app bundle"
    cd tomahawk.app
    $ROOT/../admin/mac/add-Qt-to-bundle.sh \
                   'QtCore QtGui QtXml QtNetwork QtSql QtXmlPatterns QtWebKit phonon'

    header "Running install_name_tool"
    $ROOT/../admin/mac/deposx.sh

    header "Renaming files"
    mv Contents/Resources/tomahawkSources.icns Contents/Resources/Tomahawk.icns
    mv Contents/MacOS/tomahawk Contents/MacOS/Tomahawk
#    cp $ROOT/../admin/mac/Info.plist Contents/Info.plist

    header "Copying Sparkle pubkey & framework, and qt.conf"
    cp $ROOT/../admin/mac/sparkle_pub.pem Contents/Resources
    cp -R /Library/Frameworks/Sparkle.framework Contents/Frameworks
    cp $ROOT/../admin/mac/qt.conf Contents/Resources

    header "Creating DMG"
    cd ..
    mv tomahawk.app Tomahawk.app
    $ROOT/../admin/mac/create-dmg.sh Tomahawk.app
    mv Tomahawk.app tomahawk.app

    header "Done!"
