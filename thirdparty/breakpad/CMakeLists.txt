PROJECT(breakpad)
cmake_minimum_required(VERSION 2.8)

set( CMAKE_MODULE_PATH
     ${CMAKE_MODULE_PATH}
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

add_definitions(-DSTDC_HEADERS -std=gnu++98)

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
  add_definitions(-DNDEBUG)
endif(${CMAKE_BUILD_TYPE} MATCHES "Release")

IF(UNIX)
  IF(APPLE)


  ELSE(APPLE)

    SET( breakpadSources
         client/linux/crash_generation/crash_generation_client.cc
         client/linux/crash_generation/crash_generation_server.cc
#         client/linux/sender/google_crash_report_sender.cc
         client/linux/minidump_writer/minidump_writer.cc
         client/linux/minidump_writer/linux_dumper.cc
         client/linux/handler/exception_handler.cc
         common/linux/dump_symbols.cc
         common/linux/file_id.cc
         common/linux/libcurl_wrapper.cc
         common/linux/google_crashdump_uploader.cc
         common/linux/synth_elf.cc
         common/linux/http_upload.cc
         common/linux/guid_creator.cc
         common/linux/elf_symbols_to_module.cc
         common/string_conversion.cc
         common/convert_UTF.c
         client/minidump_file_writer.cc
    )

  ENDIF(APPLE)
ENDIF(UNIX)

include_directories(.)
add_definitions(-fPIC)
add_library(tomahawk_breakpad STATIC ${breakpadSources})
target_link_libraries(tomahawk_breakpad)
