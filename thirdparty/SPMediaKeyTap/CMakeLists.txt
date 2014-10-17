set(SPMEDIAKEY-SOURCES
  SPMediaKeyTap.m
  SPInvocationGrabbing/NSObject+SPInvocationGrabbing.m
)

set(SPMEDIAKEY-HEADERS
  SPMediaKeyTap.h
  SPInvocationGrabbing/NSObject+SPInvocationGrabbing.h
)

SET_SOURCE_FILES_PROPERTIES(
  ${SPMEDIAKEY-SOURCES}
  PROPERTIES LANGUAGE C
)


ADD_LIBRARY(SPMediaKeyTap STATIC
  ${SPMEDIAKEY-SOURCES}
)
