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
PLUGINFOLDERS=`ls plugins | cut -d. -f1`

################################################################################


function import_lib
{
    echo "L \`$1'"
    cp -R -L $1 MacOS/`basename $1`
    chmod u+rw MacOS/`basename $1`
    deplib_change MacOS/`basename $1`
    deposx_change MacOS/`basename $1`
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
    install_name_tool -change /usr/local/Cellar/qjson/0.7.1/lib/libqjson.0.7.1.dylib @executable_path/libqjson.0.7.1.dylib $1
    install_name_tool -change /usr/local/lib/libechonest.1.1.dylib @executable_path/libechonest.1.1.dylib $1
    install_name_tool -change /usr/local/lib/libclucene-core.1.dylib @executable_path/libclucene-core.1.dylib $1
    install_name_tool -change /usr/local/lib/libclucene-shared.1.dylib @executable_path/libclucene-shared.1.dylib $1
    install_name_tool -change /usr/local/Cellar/taglib/1.7/lib/libtag.1.7.0.dylib @executable_path/libtag.1.7.0.dylib $1
#    install_name_tool -change /usr/local/Cellar/gloox/1.0/lib/libgloox.8.dylib @executable_path/libgloox.8.dylib $1
#    install_name_tool -change /usr/local/Cellar/libogg/1.2.0/lib/libogg.0.dylib @executable_path/libogg.0.dylib $1
#    install_name_tool -change /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbis.0.dylib @executable_path/libvorbis.0.dylib $1
#    install_name_tool -change /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbisfile.3.dylib @executable_path/libvorbisfile.3.dylib $1
#    install_name_tool -change /usr/local/Cellar/mad/0.15.1b/lib/libmad.0.dylib @executable_path/libmad.0.dylib $1
#    install_name_tool -change /usr/local/Cellar/flac/1.2.1/lib/libFLAC++.6.dylib @executable_path/libFLAC++.6.dylib $1
#    install_name_tool -change /usr/local/Cellar/flac/1.2.1/lib/libFLAC.8.dylib @executable_path/libFLAC.8.dylib $1
    install_name_tool -change /usr/local/Cellar/kde-phonon/4.5.0/lib/libphonon.4.dylib  @executable_path/libphonon.4.dylib $1
    install_name_tool -change /usr/local/Cellar/kde-phonon/4.5.0/lib/libphonon.4.5.0.dylib  @executable_path/libphonon.4.dylib $1

    install_name_tool -change $ORIGROOT/libtomahawklib.dylib @executable_path/libtomahawklib.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_sipjabber.dylib @executable_path/libtomahawk_sipjabber.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_sipgoogle.dylib @executable_path/libtomahawk_sipgoogle.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_siptwitter.dylib @executable_path/libtomahawk_siptwitter.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_sipzeroconf.dylib @executable_path/libtomahawk_sipzeroconf.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_qtweetlib.dylib @executable_path/libtomahawk_qtweetlib.dylib $1
    install_name_tool -change $ORIGROOT/libtomahawk_portfwd.dylib @executable_path/libtomahawk_portfwd.dylib $1
    install_name_tool -change $ORIGROOT/libjreen.0.dylib  @executable_path/libjreen.0.dylib $1
    install_name_tool -change /usr/local/Cellar/jreen/HEAD/lib/libjreen.dylib  @executable_path/libjreen.0.dylib $1
    install_name_tool -change /usr/local/Cellar/qca/2.0.2/lib/qca.framework/Versions/2/qca  @executable_path/../Frameworks/qca.framework/Versions/2/qca $1
    install_name_tool -change /usr/local/Cellar/gettext/0.18.1.1/lib/libintl.8.dylib  @executable_path/libintl.8.dylib $1
    install_name_tool -change /usr/local/Cellar/vlc-git/HEAD/lib/libvlc.5.dylib   @executable_path/libvlc.5.dylib $1
    install_name_tool -change /usr/local/Cellar/vlc-git/HEAD/lib/libvlccore.4.dylib  @executable_path/libvlccore.4.dylib $1

    install_name_tool -change libqjson.0.dylib @executable_path/libqjson.0.7.1.dylib $1
    install_name_tool -change libechonest.1.1.dylib @executable_path/libechonest.1.1.dylib $1
    install_name_tool -change libclucene-core.1.dylib @executable_path/libclucene-core.1.dylib $1
    install_name_tool -change libclucene-shared.1.dylib @executable_path/libclucene-shared.1.dylib $1
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

import_lib /usr/local/Cellar/qjson/0.7.1/lib/libqjson.0.7.1.dylib
import_lib /usr/local/Cellar/taglib/1.7/lib/libtag.1.7.0.dylib
#import_lib /usr/local/Cellar/gloox/1.0/lib/libgloox.8.dylib
#import_lib /usr/local/Cellar/libogg/1.2.0/lib/libogg.0.dylib
#import_lib /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbis.0.dylib
#import_lib /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbisfile.3.dylib
#import_lib /usr/local/Cellar/mad/0.15.1b/lib/libmad.0.dylib
#import_lib /usr/local/Cellar/flac/1.2.1/lib/libFLAC++.6.dylib
#import_lib /usr/local/Cellar/flac/1.2.1/lib/libFLAC.8.dylib
import_lib /usr/local/lib/libechonest.1.1.dylib
import_lib /usr/local/lib/libclucene-core.1.dylib
import_lib /usr/local/lib/libclucene-shared.1.dylib
import_lib /usr/local/Cellar/kde-phonon/4.5.0/lib/libphonon.4.dylib
import_lib /usr/local/Cellar/vlc-git/HEAD/lib/libvlc.5.dylib
import_lib /usr/local/Cellar/vlc-git/HEAD/lib/libvlccore.4.dylib
import_lib /usr/local/Cellar/gettext/0.18.1.1/lib/libintl.8.dylib

import_lib $ORIGROOT/libjreen.0.dylib
import_lib $ORIGROOT/libtomahawklib.dylib
import_lib $ORIGROOT/libtomahawk_sipjabber.dylib
import_lib $ORIGROOT/libtomahawk_sipgoogle.dylib
import_lib $ORIGROOT/libtomahawk_siptwitter.dylib
import_lib $ORIGROOT/libtomahawk_sipzeroconf.dylib
import_lib $ORIGROOT/libtomahawk_qtweetlib.dylib
import_lib $ORIGROOT/libtomahawk_portfwd.dylib

cp -R  /usr/local/Cellar/qca/2.0.2/lib/qca.framework Frameworks/
chmod 644 Frameworks/qca.framework/Versions/2/qca
deplib_change Frameworks/qca.framework/Versions/2/qca
deposx_change Frameworks/qca.framework/Versions/2/qca

# now Qt
for x in $QTLIBS
do
    echo `pwd`
#    ls -l Frameworks/$x.framework/Versions/4/$x
    deposx_change Frameworks/$x.framework/Versions/4/$x
    install_name_tool -id @executable_path/../Frameworks/$x.framework/Versions/4/$x \
                      Frameworks/$x.framework/Versions/4/$x
    deplib_change "Frameworks/$x.framework/Versions/4/$x"
done

# now VLC plugins
for x in plugins/$PLUGINFOLDERS
do
    for plugin in `ls plugins/$x | cut -f1`
    do
        echo "Fixing VLC plugin: $plugin"
        chmod 644 plugins/$x/$plugin
        deplib_change plugins/$x/$plugin
    done
done
