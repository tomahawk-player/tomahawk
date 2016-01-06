#!/bin/bash
#
# Usage: ./admin/mac/build-release-osx.sh VERSION CERT_SIGNER [--no-clean]
#
################################################################################

TARGET_NAME="Tomahawk"

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

if [ -z "$2" ]
then
    echo This script expects the version number and cert-signer as parameters, e.g. "1.0.0 John Doe"
    exit 1
fi

ROOT=`pwd`
VERSION=$1
CERT_SIGNER=$2

################################################################################

    header "Fixing and copying libraries"
    $ROOT/../admin/mac/macdeploy.py "${TARGET_NAME}.app" quiet

    cd "${TARGET_NAME}.app"

    cp $ROOT/../admin/mac/qt.conf Contents/Resources/qt.conf

    header "Fixing fonts"
    mkdir "${ROOT}/${TARGET_NAME}.app/Contents/Resources/Fonts"
    cp -R $ROOT/../data/fonts/*.ttf "${ROOT}/${TARGET_NAME}.app/Contents/Resources/Fonts"

    header "Signing bundle"
    cd ..
    if [ -f ~/sign_step.sh ];
    then
        ~/sign_step.sh "$CERT_SIGNER" "${TARGET_NAME}.app" || true
    fi

    header "Creating DMG"
    $ROOT/../admin/mac/create-dmg.sh "${TARGET_NAME}.app"
    mv "${TARGET_NAME}.dmg" "${TARGET_NAME}-$VERSION.dmg"

    header "Creating signed Sparkle update"
#     $ROOT/../admin/mac/sign_bundle.rb "${TARGET_NAME}" $VERSION ~/tomahawk_sparkle_privkey.pem

    header "Done!"
