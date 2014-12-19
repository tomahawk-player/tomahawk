#!/bin/bash
#
# Usage: ./admin/mac/build-release-osx.sh VERSION CERT_SIGNER [--no-clean]
#
################################################################################

set -e


function header {
    echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}
################################################################################

if [ -z $2 ]
then
    echo This script expects the version number and cert-signer as parameters, e.g. "1.0.0 John Doe"
    exit 1
fi

ROOT=`pwd`
VERSION=$1
CERT_SIGNER=$2

################################################################################

    header "Fixing and copying libraries"
    $ROOT/../admin/mac/macdeploy.py Tomahawk.app quiet

    cd Tomahawk.app

    cp $ROOT/../admin/mac/qt.conf Contents/Resources/qt.conf

    header "Copying Sparkle framework"
    cp -R /Library/Frameworks/Sparkle.framework Contents/Frameworks

    header "Creating DMG"
    cd ..

    header "Signing bundle"
    codesign -s "Developer ID Application: $CERT_SIGNER" -f -v ./Tomahawk.app

    $ROOT/../admin/mac/create-dmg.sh Tomahawk.app
    mv Tomahawk.dmg Tomahawk-$VERSION.dmg

    header "Creating signed Sparkle update"
    $ROOT/../admin/mac/sign_bundle.rb $VERSION ~/tomahawk_sparkle_privkey.pem

    header "Done!"
