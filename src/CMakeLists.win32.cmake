SET( CMAKE_BUILD_TYPE "Release" )

ADD_DEFINITIONS( /DNOMINMAX )
ADD_DEFINITIONS( /DWIN32_LEAN_AND_MEAN )
ADD_DEFINITIONS( -static-libgcc )
ADD_DEFINITIONS( -DUNICODE )

# Check for the availability of the Thumbbutton

check_cxx_source_compiles( "
     #include <shobjidl.h>
     int main() {
         THUMBBUTTON foo;
         return 0;
     }
     "
     HAVE_THUMBBUTTON )

SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}

    ${QTSPARKLE_LIBRARIES}

# third party shipped with tomahawk

# system libs
    "secur32.dll"
    "crypt32.dll"
    "iphlpapi.a"
    "ws2_32.dll"
    "dnsapi.dll"
    "dsound.dll"
    "winmm.dll"
    "advapi32.dll"
)
