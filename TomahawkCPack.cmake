INCLUDE( InstallRequiredSystemLibraries )

SET( CPACK_PACKAGE_CONTACT  "Dominik Schmidt <domme@tomahawk-player.org>" )

SET( CPACK_PACKAGE_FILE_NAME  tomahawk-${TOMAHAWK_VERSION} )    # Package file name without extension. Also a directory of installer  cmake-2.5.0-Linux-i686

# CPACK_GENERATOR   CPack generator to be used  STGZ;TGZ;TZ
# CPACK_INCLUDE_TOPLEVEL_DIRECTORY    Controls whether CPack adds a top-level directory, usually of the form ProjectName-Version-OS, to the top of package tree.  0 to disable, 1 to enable
# CPACK_INSTALL_CMAKE_PROJECTS    List of four values: Build directory, Project Name, Project Component, Directory in the package     /home/andy/vtk/CMake-bin;CMake;ALL;/
SET( CPACK_PACKAGE_DESCRIPTION_FILE  "${CMAKE_SOURCE_DIR}/README.md" ) # File used as a description of a project     /path/to/project/ReadMe.txt
SET( CPACK_PACKAGE_DESCRIPTION_SUMMARY  ${TOMAHAWK_DESCRIPTION_SUMMARY} ) #  Description summary of a project
# CPACK_PACKAGE_EXECUTABLES   List of pairs of executables and labels. Used by the NSIS generator to create Start Menu shortcuts.     ccmake;CMake
SET( CPACK_PACKAGE_INSTALL_DIRECTORY  ${TOMAHAWK_APPLICATION_NAME} )     # Installation directory on the target system -> C:\Program Files\fellody
SET( CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${TOMAHAWK_APPLICATION_NAME} )  # Registry key used when installing this project  CMake 2.5.0
SET( CPACK_PACKAGE_NAME  ${TOMAHAWK_APPLICATION_NAME} ) # Package name, defaults to the project name
SET( CPACK_PACKAGE_VENDOR  ${TOMAHAWK_ORGANIZATION_NAME} )   # Package vendor name
SET( CPACK_PACKAGE_VERSION_MAJOR  ${TOMAHAWK_VERSION_MAJOR} )
SET( CPACK_PACKAGE_VERSION_MINOR  ${TOMAHAWK_VERSION_MINOR} )
SET( CPACK_PACKAGE_VERSION_PATCH  ${TOMAHAWK_VERSION_PATCH} )

# CPACK_SOURCE_GENERATOR  List of generators used for the source package  TGZ;TZ

SET( CPACK_SOURCE_GENERATOR TGZ )
SET( CPACK_SOURCE_IGNORE_FILES "/\\\\.git/" ".*~$" ".kate-swp$" "/build_dir/" "/clang/" "/gcc/" "/build/" "/win/" ) # Pattern of files in the source tree that won't be packaged
SET( CPACK_SOURCE_PACKAGE_FILE_NAME tomahawk-${TOMAHAWK_VERSION} ) # Name of the source package
# CPACK_SOURCE_STRIP_FILES    List of files in the source tree that will be stripped. Starting with CMake 2.6.0 CPACK_SOURCE_STRIP_FILES will be a boolean variable which enables stripping of all files (a list of files evaluates to TRUE in CMake, so this change is compatible).
# CPACK_STRIP_FILES   List of files to be stripped. Starting with CMake 2.6.0 CPACK_STRIP_FILES will be a boolean variable which enables stripping of all files (a list of files evaluates to TRUE in CMake, so this change is compatible).   bin/ccmake;bin/cmake;bin/cpack;bin/ctest
# CPACK_SYSTEM_NAME   System name, defaults to the value of ${CMAKE_SYSTEM_NAME}.     Linux-i686

# Advanced settings
# CPACK_CMAKE_GENERATOR   What CMake generator should be used if the project is CMake project. Defaults to the value of CMAKE_GENERATOR.  Unix Makefiles
SET( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt" ) # License file for the project, used by the STGZ, NSIS, and PackageMaker generators.  /home/andy/vtk/CMake/Copyright.txt
# CPACK_RESOURCE_FILE_README  ReadMe file for the project, used by PackageMaker generator.    /home/andy/vtk/CMake/Templates/CPack.GenericDescription.txt
# CPACK_RESOURCE_FILE_WELCOME     Welcome file for the project, used by PackageMaker generator.   /home/andy/vtk/CMake/Templates/CPack.GenericWelcome.txt
SET( CPACK_PACKAGE_VERSION  ${TOMAHAWK_VERSION} )

SET( CPACK_TOPLEVEL_TAG "narf" ) # Directory for the installed files.  - needed to provide anything to avoid an error# CPACK_INSTALL_COMMANDS  Extra commands to install components.
# CPACK_INSTALL_DIRECTORIES   Extra directories to install.
# CPACK_MONOLITHIC_INSTALL    When set disables the component-based installer.
# CPACK_PACKAGING_INSTALL_PREFIX  Sets the default root that the generated package installs into, '/usr' is the default for the debian and redhat generators  /usr/local

##
# INSTALL DEPS
##



# Set the options file that needs to be included inside CMakeCPackOptions.cmake
#SET(QT_DIALOG_CPACK_OPTIONS_FILE ${CMake_BINARY_DIR}/Source/QtDialog/QtDialogCPack.cmake)
configure_file("${CMAKE_SOURCE_DIR}/CPackOptions.cmake.in"
    "${CMAKE_BINARY_DIR}/CPackOptions.cmake" @ONLY)
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackOptions.cmake") # File included at cpack time, once per generator after setting CPACK_GENERATOR to the actual generator being used; allows per-generator setting of CPACK_* variables at cpack time.  ${PROJECT_BINARY_DIR}/CPackOptions.cmake
include(CPack)
