find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBVLC QUIET libvlc)
set(LIBVLC_DEFINITIONS ${PC_LIBVLC_CFLAGS_OTHER})

find_path(LIBVLC_INCLUDE_DIR vlc/vlc.h
    HINTS
        ${PC_LIBVLC_INCLUDEDIR}
        ${PC_LIBVLC_INCLUDE_DIRS}
        /usr/local/opt/vlc/include
)

find_library(LIBVLC_LIBRARY NAMES vlc libvlc
    HINTS
        ${PC_LIBVLC_LIBDIR}
        ${PC_LIBVLC_LIBRARY_DIRS}
        /usr/local/opt/vlc/lib
)


set(LIBVLC_VERSION ${PC_LIBVLC_VERSION})

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
int main(int argc, char *argv[]) {
    libvlc_meta_t meta = libvlc_meta_AlbumArtist;
}"
HAVE_VLC_ALBUMARTIST)

find_package_handle_standard_args(LibVLC
    REQUIRED_VARS LIBVLC_LIBRARY LIBVLC_INCLUDE_DIR
    VERSION_VAR LIBVLC_VERSION
)
