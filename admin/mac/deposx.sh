#!/bin/sh
# author: max@last.fm, chris@last.fm
################################################################################


if [ -z $QTDIR ]
then
    echo QTDIR must be set
    exit 1
fi

if [ -z $QTVERSION ]
then
    echo QTVERSION must be set
    exit 1
fi

cd ..
ORIGROOT=`pwd`
cd -

cd Contents

QTLIBS=`ls Frameworks | cut -d. -f1`
LIBS=`cd MacOS && ls -fR1 | grep dylib`
################################################################################


function import_lib
{
    echo "L \`$1'"
    cp -R -L $1 MacOS/`basename $1`
    chmod u+rw MacOS/`basename $1`
    deplib_change MacOS/`basename $1`
}

function deposx_change 
{
    echo "D \`$1'"
    echo $QTDIR
    
    for y in $QTLIBS
    do
        install_name_tool -change $QTDIR/lib/$y.framework/Versions/4/$y \
                          @executable_path/../Frameworks/$y.framework/Versions/4/$y \
                          "$1"

        install_name_tool -change /usr/local/Cellar/qt/$QTVERSION/lib/$y.framework/Versions/4/$y \
                          @executable_path/../Frameworks/$y.framework/Versions/4/$y \
                          "$1"                          
    done
    
    for y in $LIBS
    do
        install_name_tool -change $y \
                          @executable_path/$y \
                          "$1"
    done
}

function deplib_change
{
    install_name_tool -change /usr/local/Cellar/liblastfm/0.3.1/lib/liblastfm.0.dylib @executable_path/liblastfm.0.dylib $1
    install_name_tool -change libqjson.0.dylib @executable_path/libqjson.0.dylib $1
    install_name_tool -change /usr/local/lib/libechonest.1.1.dylib @executable_path/libechonest.1.1.dylib $1
    install_name_tool -change /usr/local/Cellar/clucene/0.9.21/lib/libclucene.0.dylib @executable_path/libclucene.0.dylib $1
    install_name_tool -change /usr/local/Cellar/gloox/1.0/lib/libgloox.8.dylib @executable_path/libgloox.8.dylib $1
    install_name_tool -change /usr/local/Cellar/taglib/1.6.3/lib/libtag.1.dylib @executable_path/libtag.1.dylib $1
    install_name_tool -change /usr/local/Cellar/libogg/1.2.0/lib/libogg.0.dylib @executable_path/libogg.0.dylib $1
    install_name_tool -change /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbis.0.dylib @executable_path/libvorbis.0.dylib $1
    install_name_tool -change /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbisfile.3.dylib @executable_path/libvorbisfile.3.dylib $1
    install_name_tool -change /usr/local/Cellar/mad/0.15.1b/lib/libmad.0.dylib @executable_path/libmad.0.dylib $1
    install_name_tool -change /usr/local/Cellar/flac/1.2.1/lib/libFLAC++.6.dylib @executable_path/libFLAC++.6.dylib $1
    install_name_tool -change /usr/local/Cellar/flac/1.2.1/lib/libFLAC.8.dylib @executable_path/libFLAC.8.dylib $1
    install_name_tool -change $ORIGROOT/src/libtomahawk/libtomahawklib.dylib @executable_path/libtomahawklib.dylib $1
    install_name_tool -change $ORIGROOT/libsip_jabber.dylib @executable_path/libsip_jabber.dylib $1
    install_name_tool -change $ORIGROOT/libsip_zeroconf.dylib @executable_path/libsip_zeroconf.dylib $1
    install_name_tool -change $ORIGROOT/thirdparty/jdns/libtomahawk_jdns.dylib @executable_path/libtomahawk_jdns.dylib $1
}

################################################################################


# first all libraries and executables
find MacOS -type f -a -perm -100 | while read x
do
    echo $x
    y=$(file "$x" | grep 'Mach-O')
    deposx_change "$x"
    deplib_change "$x"
done

import_lib /usr/local/Cellar/qjson/0.7.1/lib/libqjson.0.dylib
import_lib /usr/local/Cellar/liblastfm/0.3.1/lib/liblastfm.0.dylib
import_lib /usr/local/Cellar/gloox/1.0/lib/libgloox.8.dylib
import_lib /usr/local/Cellar/taglib/1.6.3/lib/libtag.1.dylib
import_lib /usr/local/Cellar/libogg/1.2.0/lib/libogg.0.dylib
import_lib /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbis.0.dylib
import_lib /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbisfile.3.dylib
import_lib /usr/local/Cellar/mad/0.15.1b/lib/libmad.0.dylib
import_lib /usr/local/Cellar/flac/1.2.1/lib/libFLAC++.6.dylib
import_lib /usr/local/Cellar/flac/1.2.1/lib/libFLAC.8.dylib
import_lib /usr/local/lib/libechonest.1.1.dylib
import_lib /usr/local/Cellar/clucene/0.9.21/lib/libclucene.0.dylib

import_lib ../../libsip_jabber.dylib
import_lib ../../libsip_zeroconf.dylib
import_lib ../../src/libtomahawk/libtomahawklib.dylib
import_lib ../../thirdparty/jdns/libtomahawk_jdns.dylib

deposx_change MacOS/libqjson.0.dylib
deposx_change MacOS/liblastfm.0.dylib
deposx_change MacOS/libclucene.0.dylib
deposx_change MacOS/libechonest.1.1.dylib
deposx_change MacOS/libsip_jabber.dylib
deposx_change MacOS/libsip_zeroconf.dylib
deposx_change MacOS/libtomahawklib.dylib
deposx_change MacOS/libtomahawk_jdns.dylib

# now Qt
for x in $QTLIBS
do
    echo `pwd`
#    ls -l Frameworks/$x.framework/Versions/4/$x
    deposx_change Frameworks/$x.framework/Versions/4/$x
    install_name_tool -id @executable_path/../Frameworks/$x.framework/Versions/4/$x \
                      Frameworks/$x.framework/Versions/4/$x
done
