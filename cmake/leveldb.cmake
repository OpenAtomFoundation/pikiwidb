# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

FETCHCONTENT_DECLARE(
    leveldb
    GIT_REPOSITORY https://github.com/google/leveldb.git
    GIT_TAG main
)
SET(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
SET(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
FETCHCONTENT_MAKEAVAILABLE(leveldb)

SET(LEVELDB_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-src/include)
SET(LEVELDB_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-build/libleveldb.a)