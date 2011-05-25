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

if [ -z $1 ]
then
    echo This script expects the version number as a parameter, e.g. 1.0.0
    exit 1
fi

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
export QTVERSION='4.7.3'
################################################################################


CLEAN='1'
BUILD='1'
NOTQUICK='1'
CREATEDMG='1'
VERSION=$1

    header "Adding Qt to app bundle"
    cd tomahawk.app
    $ROOT/../admin/mac/add-Qt-to-bundle.sh \
                   'QtCore QtGui QtXml QtNetwork QtSql QtXmlPatterns QtWebKit phonon'
#                   'QtCore QtGui QtXml QtNetwork QtSql QtXmlPatterns QtWebKit QtDbus phonon'

    header "Renaming files"
    mv Contents/Resources/tomahawkSources.icns Contents/Resources/Tomahawk.icns
    mv Contents/MacOS/tomahawk Contents/MacOS/Tomahawk
#    cp $ROOT/../admin/mac/Info.plist Contents/Info.plist

    header "Copying VLC plugins into bundle"
    mkdir -p Contents/plugins
    cp -R /usr/local/Cellar/vlc-git/HEAD/lib/vlc/plugins/ Contents/plugins
    rm -rf Contents/plugins/video_* Contents/plugins/gui Contents/plugins/*/libold* Contents/plugins/*/libvcd* Contents/plugins/*/libdvd* \
           Contents/plugins/*/liblibass* Contents/plugins/*/libx264* Contents/plugins/*/libschroe* Contents/plugins/*/liblibmpeg2* \
           Contents/plugins/*/libstream_out_* Contents/plugins/*/libmjpeg_plugin* Contents/plugins/*/libh264_plugin* Contents/plugins/*/libzvbi_plugin* Contents/plugins/*/lib*sub*

    header "Running install_name_tool"
    $ROOT/../admin/mac/deposx.sh

    header "Copying Sparkle pubkey & framework, and qt.conf"
    cp $ROOT/../admin/mac/sparkle_pub.pem Contents/Resources
    cp -R /Library/Frameworks/Sparkle.framework Contents/Frameworks
    cp $ROOT/../admin/mac/qt.conf Contents/Resources

    header "Adding spotify resolver to bundle if spotify_tomahawkresolver found in $ROOT"
    if [ -e $ROOT/spotify_tomahawkresolver ]
       then
        header "Found, so adding spotify resolver."ac
        $ROOT/../admin/mac/add-spotify.sh $ROOT/spotify_tomahawkresolver
    fi

    header "Creating DMG"
    cd ..
    mv tomahawk.app Tomahawk.app
    $ROOT/../admin/mac/create-dmg.sh Tomahawk.app
    mv Tomahawk.dmg Tomahawk-$VERSION.dmg
    
    header "Creating signed Sparkle update"
    $ROOT/../admin/mac/sign_bundle.rb $VERSION ~/tomahawk_sparkle_privkey.pem
    mv Tomahawk.app tomahawk.app

    header "Done!"
