ADD_DEFINITIONS( -DNOMINMAX )
ADD_DEFINITIONS( -DWIN32_LEAN_AND_MEAN )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc" )
ADD_DEFINITIONS( -DUNICODE )

SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
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

if(QTSPARKLE_FOUND)
    list(APPEND OS_SPECIFIC_LINK_LIBRARIES ${QTSPARKLE_LIBRARIES})
endif()


include(CheckCXXSourceCompiles)

check_cxx_source_compiles( "#include <fstream>
                            int main(){
                                std::ofstream stream(L\"Test\");
                                return 0;
                            }"
                            OFSTREAM_CAN_OPEN_WCHAR_FILE_NAMES)