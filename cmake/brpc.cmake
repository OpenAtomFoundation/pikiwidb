# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

if (CMAKE_VERSION VERSION_GREATER_EQUAL "2.8.12")
  cmake_policy(SET CMP0135 NEW)
endif()

include(cmake/gflags.cmake)
include(cmake/protobuf.cmake)
include(cmake/leveldb.cmake)

FETCHCONTENT_DECLARE(
        brpc
        GIT_REPOSITORY https://github.com/apache/brpc.git
        GIT_TAG 1.7.0
        # SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/brpc-src
        # BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/brpc-build/output/include
)

set(BRPC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/brpc-build)
set(CMAKE_PREFIX_PATH ${BRPC_BUILD_DIR}/output ${CMAKE_PREFIX_PATH})

SET(BRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(BRPC_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
SET(BRPC_INSTALL OFF CACHE BOOL "" FORCE)
FETCHCONTENT_MAKEAVAILABLE(brpc)

add_library(brpc STATIC IMPORTED)
set_target_properties(brpc PROPERTIES IMPORTED_LOCATION ${BRPC_BUILD_DIR}/output/lib/libbrpc.a)
# target_link_libraries(brpc gflags_static protobuf leveldb)
message(${BRPC_BUILD_DIR})