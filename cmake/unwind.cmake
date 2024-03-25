# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

INCLUDE(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(unwind
  libunwind/libunwind v1.7.2
  SHA256=f39929bff6ebd4426e806f0e834e077f2dc3c16fa19dbfb8996a1c93b3caf8cb
)

FETCHCONTENT_GETPROPERTIES(unwind)
IF (NOT unwind_POPULATED)
  FETCHCONTENT_POPULATE(unwind) 
  
  EXECUTE_PROCESS(COMMAND autoreconf -i
    WORKING_DIRECTORY ${unwind_SOURCE_DIR}
  )
  EXECUTE_PROCESS(COMMAND ${unwind_SOURCE_DIR}/configure CC=${CMAKE_C_COMPILER} -C --enable-static=yes --enable-shared=no --enable-minidebuginfo=no --enable-zlibdebuginfo=no --disable-documentation --disable-tests
    WORKING_DIRECTORY ${unwind_BINARY_DIR}
  ) 
  ADD_CUSTOM_TARGET(make_unwind
    COMMAND ${MAKE_COMMAND}
    WORKING_DIRECTORY ${unwind_BINARY_DIR}
    BYPRODUCTS ${unwind_BINARY_DIR}/src/.libs/libunwind.a
  )
ENDIF ()

ADD_LIBRARY(unwind INTERFACE)
TARGET_INCLUDE_DIRECTORIES(unwind INTERFACE $<BUILD_INTERFACE:${unwind_BINARY_DIR}/include> $<BUILD_INTERFACE:${unwind_SOURCE_DIR}/include>)
TARGET_LINK_LIBRARIES(unwind INTERFACE $<BUILD_INTERFACE:${unwind_BINARY_DIR}/src/.libs/libunwind.a>)
ADD_DEPENDENCIES(unwind make_unwind)
