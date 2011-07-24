#!/bin/bash
echo "Remove old vlc dir..."
#rm -vf vlc-*.7z
rm -rf vlc/

echo "Download specified binary..."
#wget -c "http://downloads.sourceforge.net/project/vlc/1.1.9/win32/vlc-1.1.9-win32.7z?r=http%3A%2F%2Fwww.videolan.org%2Fvlc%2Fdownload-windows.html&ts=1306272584&use_mirror=leaseweb"
wget -c "http://download.tomahawk-player.org/tomahawk-vlc-0.1.zip"

echo "Extract binary..."
#7z x vlc-*.7z
#mv -v vlc-*/ vlc/
unzip tomahawk-vlc-0.1.zip

#echo "Strip unneeded plugins from vlc/plugins..."
# cd vlc/plugins/
# rm -rvf video_*/ gui/ */libold* */libvcd* */libdvd* */liblibass* */libx264* */libschroe* */liblibmpeg2* \
#      */libstream_out_* */libmjpeg_plugin* */libh264_plugin* */libzvbi_plugin* */lib*sub* \
#      services_discover/ visualization/ control/ misc/


echo "Downloaded and stripped VLC"

