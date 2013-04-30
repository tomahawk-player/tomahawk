# - Try to find AccountsQt
# Once done this will define
#  ACCOUNTSQT_FOUND - System has libaccounts-qt
#  ACCOUNTSQT_INCLUDE_DIRS - The libaccounts-qt include directories
#  ACCOUNTSQT_LIBRARIES - The libraries needed to use libaccounts-qt
#
#  Copyright 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>
 
find_package(PkgConfig)

find_path(ACCOUNTSQT_INCLUDE_DIR Accounts/Account
          HINTS ${ACCOUNTSQT_INCLUDEDIR} ${ACCOUNTSQT_INCLUDE_DIRS} ${CMAKE_INSTALL_PREFIX}/include
          PATH_SUFFIXES accounts-qt )

find_library(ACCOUNTSQT_LIBRARY NAMES accounts-qt libaccounts-qt
    HINTS ${ACCOUNTSQT_LIBDIR} ${ACCOUNTSQT_LIBRARY_DIRS} ${CMAKE_INSTALL_PREFIX}/lib64)

set(ACCOUNTSQT_LIBRARIES ${ACCOUNTSQT_LIBRARY} )
set(ACCOUNTSQT_INCLUDE_DIRS ${ACCOUNTSQT_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set  ACCOUNTSQT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libaccounts-qt  DEFAULT_MSG
                                  ACCOUNTSQT_LIBRARY ACCOUNTSQT_INCLUDE_DIR)

mark_as_advanced(ACCOUNTSQT_INCLUDE_DIR ACCOUNTSQT_LIBRARY )
