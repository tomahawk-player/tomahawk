# Find libphonon
# Once done this will define
#
#  PHONON_FOUND    - system has Phonon Library
#  PHONON_INCLUDES - the Phonon include directory
#  PHONON_LIBS     - link these to use Phonon
#  PHONON_VERSION  - the version of the Phonon Library

# Copyright (c) 2008, Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

if( TOMAHAWK_QT5 )
    find_package(Phonon4Qt5 NO_MODULE)
    set(Phonon_FOUND ${Phonon4Qt5_FOUND})
    set(Phonon_DIR ${Phonon4Qt5_DIR})
else()
    find_package(Phonon NO_MODULE)
endif()

find_package_handle_standard_args(Phonon DEFAULT_MSG  Phonon_DIR )
