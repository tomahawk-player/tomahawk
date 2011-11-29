#!/bin/bash
mkdir -p vlc/

if [ "$1" = "-c" ] ; then
echo "Continuing last download.."
rm -rvf vlc/prefix/
else
echo "Remove old vlc dir..."
rm -rvf vlc/*
fi

cd vlc/

echo "Download phonon archive..."
#wget -c "http://downloads.sourceforge.net/project/vlc/1.1.9/win32/vlc-1.1.9-win32.7z?r=http%3A%2F%2Fwww.videolan.org%2Fvlc%2Fdownload-windows.html&ts=1306272584&use_mirror=leaseweb"
#wget -c "http://download.tomahawk-player.org/tomahawk-vlc-0.1.zip"
#wget -c http://people.videolan.org/~jb/phonon/phonon-vlc-last.7z
wget -c http://people.videolan.org/~jb/phonon/phonon_phonon-vlc_20111128.7z

echo "Extract binary..."
7z x phonon*.7z
#mv -v vlc-*/ vlc/
#unzip tomahawk-vlc-0.1.zip

echo "Download phonon_vlc_no_video.dll..."
wget -c http://people.videolan.org/~jb/phonon/phonon_vlc_no_video.dll
cp -v phonon_vlc_no_video.dll prefix/bin/phonon_backend/phonon_vlc.dll

echo "Strip unneeded plugins from vlc/plugins..."
cd prefix/bin/plugins
rm -rvf libold* libvcd* libdvd* liblibass* libx264* libschroe* liblibmpeg2* \
    libstream_out_* libmjpeg_plugin* libh264_plugin* libzvbi_plugin* lib*sub* \
    *qt4* *skins2* libaccess_bd_plugin.dll \
    libaudiobargraph_* libball_plugin.dll \
    libdirac_plugin.dll \
    libgnutls_plugin.dll \
    libcaca_plugin.dll \
    libfreetype_plugin.dll \
    libaccess_output_shout_plugin.dll \
    libremoteosd_plugin.dll \
    libsdl_image_plugin.dll \
    libvout_sdl_plugin.dll \
    libpng_plugin.dll \
    libgoom_plugin.dll \
    libatmo_plugin.dll \
    libmux_ts_plugin.dll \
    libkate_plugin.dll \
    libtaglib_plugin.dll


# this is for vlc-1.2
# rm -rvf video_*/ gui/ */libold* */libvcd* */libdvd* */liblibass* */libx264* */libschroe* */liblibmpeg2* \
#      */libstream_out_* */libmjpeg_plugin* */libh264_plugin* */libzvbi_plugin* */lib*sub* \
#      services_discover/ visualization/ control/ misc/


echo "Downloaded and stripped VLC"

