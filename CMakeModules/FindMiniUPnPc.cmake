# Try to find miniupnpc (https://github.com/miniupnp/miniupnp/)
#
# This will define
# LIBMINIUPNPC_FOUND
# LIBMINIUPNPC_INCLUDE_DIRS
# LIBMINIUPNPC_LIBRARIES

find_path(LIBMINIUPNPC_INCLUDE_DIR miniupnpc/miniupnpc.h
    PATH_SUFFIXES miniupnpc
)

find_library(LIBMINIUPNPC_LIBRARY
    NAMES miniupnpc
)

set(LIBMINIUPNPC_INCLUDE_DIRS ${LIBMINIUPNPC_INCLUDE_DIR})
set(LIBMINIUPNPC_LIBRARIES ${LIBMINIUPNPC_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libminiupnpc DEFAULT_MSG LIBMINIUPNPC_LIBRARY LIBMINIUPNPC_INCLUDE_DIR)

mark_as_advanced(LIBMINIUPNPC_INCLUDE_DIR LIBMINIUPNPC_LIBRARY)
