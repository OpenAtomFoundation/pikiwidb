# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

INCLUDE(FetchContent)

MACRO(parse_var arg key value)
  STRING(REGEX REPLACE "^(.+)=(.+)$" "\\1;\\2" REGEX_RESULT ${arg})
  LIST(GET REGEX_RESULT 0 ${key})
  LIST(GET REGEX_RESULT 1 ${value})
ENDMACRO()

FUNCTION(FetchContent_MakeAvailableWithArgs dep)
  IF (NOT ${dep}_POPULATED)
    FETCHCONTENT_POPULATE(${dep})

    FOREACH(arg IN LISTS ARGN)
      parse_var(${arg} key value)
      SET(${key}_OLD ${${key}})
      SET(${key} ${value} CACHE INTERNAL "")
    ENDFOREACH()

    ADD_SUBDIRECTORY(${${dep}_SOURCE_DIR} ${${dep}_BINARY_DIR})

    FOREACH(arg IN LISTS ARGN)
      parse_var(${arg} key value)
      SET(${key} ${${key}_OLD} CACHE INTERNAL "")
    ENDFOREACH()
  ENDIF ()
ENDFUNCTION()

FUNCTION(FetchContent_DeclareWithMirror dep url hash)
  FetchContent_Declare(${dep}
    URL ${DEPS_FETCH_PROXY}${url}
    URL_HASH ${hash}
  )
ENDFUNCTION()

FUNCTION(FetchContent_DeclareGitHubWithMirror dep repo tag hash)
  FetchContent_DeclareWithMirror(${dep}
    https://github.com/${repo}/archive/${tag}.zip
    ${hash}
  )
ENDFUNCTION()
