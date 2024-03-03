# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

INCLUDE(cmake/utils.cmake)

SET(MY_BUILD_TYPE ${CMAKE_BUILD_TYPE})
SET(CMAKE_BUILD_TYPE ${THIRD_PARTY_BUILD_TYPE})

set(MY_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE ${THIRD_PARTY_BUILD_TYPE})

FetchContent_DeclareGitHubWithMirror(gflags
  gflags/gflags v2.2.2
  SHA256=19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5
)

FetchContent_MakeAvailableWithArgs(gflags
  GFLAGS_NAMESPACE=gflags
  BUILD_STATIC_LIBS=ON
  BUILD_SHARED_LIBS=OFF
  BUILD_gflags_LIB=ON
  BUILD_gflags_nothreads_LIB=OFF
  BUILD_TESTING=OFF
)

FIND_PACKAGE(Threads REQUIRED)

TARGET_LINK_LIBRARIES(gflags_static Threads::Threads)

SET(GFLAGS_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/_deps/gflags-build/include)
SET(GFLAGS_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/_deps/gflags-build/libgflags.a)
SET(GFLAGS_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/gflags-build/libgflags.a)

SET(CMAKE_BUILD_TYPE ${MY_BUILD_TYPE})