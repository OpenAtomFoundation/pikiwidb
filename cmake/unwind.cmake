# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

include(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(unwind
  libunwind/libunwind v1.7.2
  SHA256=f39929bff6ebd4426e806f0e834e077f2dc3c16fa19dbfb8996a1c93b3caf8cb
)

FetchContent_GetProperties(unwind)
if(NOT unwind_POPULATED)
  FetchContent_Populate(unwind)

  execute_process(COMMAND autoreconf -i
    WORKING_DIRECTORY ${unwind_SOURCE_DIR}
  )
  execute_process(COMMAND ${unwind_SOURCE_DIR}/configure CC=${CMAKE_C_COMPILER} -C --enable-static=yes --enable-shared=no --enable-minidebuginfo=no --enable-zlibdebuginfo=no --disable-documentation --disable-tests
    WORKING_DIRECTORY ${unwind_BINARY_DIR}
  )
  add_custom_target(make_unwind
    COMMAND ${MAKE_COMMAND}
    WORKING_DIRECTORY ${unwind_BINARY_DIR}
    BYPRODUCTS ${unwind_BINARY_DIR}/src/.libs/libunwind.a
  )
endif()

add_library(unwind INTERFACE)
target_include_directories(unwind INTERFACE $<BUILD_INTERFACE:${unwind_BINARY_DIR}/include> $<BUILD_INTERFACE:${unwind_SOURCE_DIR}/include>)
target_link_libraries(unwind INTERFACE $<BUILD_INTERFACE:${unwind_BINARY_DIR}/src/.libs/libunwind.a>)
add_dependencies(unwind make_unwind)
