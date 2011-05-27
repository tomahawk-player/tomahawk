#!/bin/bash
echo "Remove old vlc dir..."
#rm -vf vlc-*.7z
rm -rf vlc/

echo "Download specified binary..."
#wget -c "http://downloads.sourceforge.net/project/vlc/1.1.9/win32/vlc-1.1.9-win32.7z?r=http%3A%2F%2Fwww.videolan.org%2Fvlc%2Fdownload-windows.html&ts=1306272584&use_mirror=leaseweb"
wget -c "http://nightlies.videolan.org/build/win32/trunk-20110524-1321/vlc-1.2.0-git-20110524-1321-win32.7z"

echo "Extract binary..."
7z x vlc-*.7z
mv -v vlc-*/ vlc/

echo "Strip unneeded plugins from vlc/plugins..."
cd vlc/plugins/
rm -rvf video_*/ gui/ */libold* */libvcd* */libdvd* */liblibass* */libx264* */libschroe* */liblibmpeg2* \
     */libstream_out_* */libmjpeg_plugin* */libh264_plugin* */libzvbi_plugin* */lib*sub*

echo "Downloaded and stripped VLC"

