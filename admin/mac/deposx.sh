#!/bin/sh
# author: max@last.fm, chris@last.fm
################################################################################


if [ -z $QTDIR ]
then
    echo QTDIR must be set
    exit 1
fi

cd Contents

QTLIBS=`ls Frameworks | cut -d. -f1`
LIBS=`cd MacOS && ls -fR1 | grep dylib`
################################################################################


function deposx_change 
{
    echo "D \`$1'"
    echo $QTDIR
    

    for y in $QTLIBS
    do
        install_name_tool -change $QTDIR/lib/$y.framework/Versions/4/$y \
                          @executable_path/../Frameworks/$y.framework/Versions/4/$y \
                          "$1"

        install_name_tool -change $QTDIR/Cellar/qt/4.6.2/lib/$y.framework/Versions/4/$y \
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
################################################################################


# first all libraries and executables
find MacOS -type f -a -perm -100 | while read x
do
    echo $x
    y=$(file "$x" | grep 'Mach-O')
    test -n "$y" && deposx_change "$x"
    
    install_name_tool -change liblastfm.0.dylib @executable_path/liblastfm.0.dylib $x
    install_name_tool -change /usr/local/Cellar/gloox/1.0/lib/libgloox.8.dylib @executable_path/libgloox.8.dylib $x
    install_name_tool -change /usr/local/lib/libgloox.8.dylib @executable_path/libgloox.8.dylib $x
    install_name_tool -change /usr/local/Cellar/taglib/1.6/lib/libtag.1.dylib @executable_path/libtag.1.dylib $x
    install_name_tool -change /usr/local/Cellar/libogg/1.2.0/lib/libogg.0.dylib @executable_path/libogg.0.dylib $x
    install_name_tool -change /usr/local/Cellar/libvorbis/1.3.1/lib/libvorbisfile.3.dylib @executable_path/libvorbisfile.3.dylib $x
    install_name_tool -change /usr/local/Cellar/mad/0.15.1b/lib/libmad.0.dylib @executable_path/libmad.0.dylib $x
done

deposx_change MacOS/libqjson.0.7.1.dylib
deposx_change MacOS/liblastfm.0.dylib

# now Qt
for x in $QTLIBS
do
    echo `pwd`
#    ls -l Frameworks/$x.framework/Versions/4/$x
    deposx_change Frameworks/$x.framework/Versions/4/$x
    install_name_tool -id @executable_path/../Frameworks/$x.framework/Versions/4/$x \
                      Frameworks/$x.framework/Versions/4/$x
done
