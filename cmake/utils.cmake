# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

include(FetchContent)

macro(parse_var arg key value)
  string(REGEX REPLACE "^(.+)=(.+)$" "\\1;\\2" REGEX_RESULT ${arg})
  list(GET REGEX_RESULT 0 ${key})
  list(GET REGEX_RESULT 1 ${value})
endmacro()

function(FetchContent_MakeAvailableWithArgs dep)
  if(NOT ${dep}_POPULATED)
    FetchContent_Populate(${dep})

    foreach(arg IN LISTS ARGN)
      parse_var(${arg} key value)
      set(${key}_OLD ${${key}})
      set(${key} ${value} CACHE INTERNAL "")
    endforeach()

    add_subdirectory(${${dep}_SOURCE_DIR} ${${dep}_BINARY_DIR} EXCLUDE_FROM_ALL)

    foreach(arg IN LISTS ARGN)
      parse_var(${arg} key value)
      set(${key} ${${key}_OLD} CACHE INTERNAL "")
    endforeach()
  endif()
endfunction()

function(FetchContent_DeclareWithMirror dep url hash)
  FetchContent_Declare(${dep}
    URL ${DEPS_FETCH_PROXY}${url}
    URL_HASH ${hash}
  )
endfunction()

function(FetchContent_DeclareGitHubWithMirror dep repo tag hash)
  FetchContent_DeclareWithMirror(${dep}
    https://github.com/${repo}/archive/${tag}.zip
    ${hash}
  )
endfunction()
