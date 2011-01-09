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
export QTVERSION='4.7.1'
################################################################################


CLEAN='1'
BUILD='1'
NOTQUICK='1'
CREATEDMG='1'

    header addQt
    cd tomahawk.app
    $ROOT/admin/mac/add-Qt-to-bundle.sh \
                   'QtCore QtGui QtXml QtNetwork QtSql'

    header deposx
    $ROOT/admin/mac/deposx.sh
    
    header Done!

