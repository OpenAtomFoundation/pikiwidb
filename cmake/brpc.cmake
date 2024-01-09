# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.


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
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/build/_deps/brpc-build/output ${CMAKE_PREFIX_PATH})
# set(BRPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/build/_deps/brpc-build/output/include)

SET(BRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(BRPC_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
SET(BRPC_INSTALL OFF CACHE BOOL "" FORCE)
FETCHCONTENT_MAKEAVAILABLE(brpc)

# target_link_libraries(brpc gflags_static protobuf leveldb)