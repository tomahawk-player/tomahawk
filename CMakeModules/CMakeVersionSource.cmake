# Try to identify the current development source version.
set(CMAKE_VERSION_SOURCE "")
if(EXISTS ${CMAKE_SOURCE_DIR}/.git/HEAD)
  find_program(GIT_EXECUTABLE NAMES git git.cmd)
  mark_as_advanced(GIT_EXECUTABLE)
  if(GIT_EXECUTABLE)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --verify -q --short=7 HEAD
      OUTPUT_VARIABLE head
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    if(head)
      set(branch "")
      execute_process(
        COMMAND ${GIT_EXECUTABLE} name-rev HEAD
        OUTPUT_VARIABLE branch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
      string(REGEX REPLACE "HEAD " "" branch "${branch}")
      set(CMAKE_VERSION_SOURCE "git-${branch}-${head}")
      execute_process(
        COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
      execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
        OUTPUT_VARIABLE dirty
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
      if(dirty)
        set(CMAKE_VERSION_SOURCE "${CMAKE_VERSION_SOURCE}-dirty")
      endif()
    endif()
  endif()
elseif(EXISTS ${CMAKE_SOURCE_DIR}/CVS/Repository)
  file(READ ${CMAKE_SOURCE_DIR}/CVS/Repository repo)
  set(branch "")
  if("${repo}" MATCHES "\\.git/")
    string(REGEX REPLACE ".*\\.git/([^\r\n]*).*" "-\\1" branch "${repo}")
  endif()
  set(CMAKE_VERSION_SOURCE "cvs${branch}")
endif()
