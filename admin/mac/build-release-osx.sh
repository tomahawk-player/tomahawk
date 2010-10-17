#!/bin/bash
#
# Usage: dist/build-relese-osx.sh [-j] [--no-clean]
#
# Adding the -j parameter results in building a japanese version.
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
QTDIR=`dirname $QTDIR`
QTDIR=`dirname $QTDIR`
test -L "$QTDIR" && QTDIR=`readlink $QTDIR`

export QMAKESPEC='macx-g++'
export QTDIR
export VERSION
################################################################################


CLEAN='1'
BUILD='1'
NOTQUICK='1'
CREATEDMG='1'

    header addQt
    cd tomahawk.app
#    $ROOT/admin/mac/add-Qt-to-bundle.sh \
#                   'QtCore QtGui QtXml QtNetwork QtSql'

    header deposx
    $ROOT/admin/mac/deposx.sh
    
    header Done!

