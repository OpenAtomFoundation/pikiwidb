# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
include_guard()

FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.6.7
)

FetchContent_GetProperties(rocksdb)

if(NOT rocksdbPOPULATED)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "")
    set(CMAKE_WITH_TESTS OFF CACHE BOOL "")
    set(CMAKE_WITH_BENCHMARK OFF CACHE BOOL "")
    set(CMAKE_WITH_BENCHMARK_TOOLS OFF CACHE BOOL "")
    set(CMAKE_WITH_TOOLS OFF CACHE BOOL "")
    set(CMAKE_WITH_CORE_TOOLS OFF CACHE BOOL "")
    set(CMAKE_WITH_TRACE_TOOLS OFF CACHE BOOL "")
    set(CMAKE_WITH_EXAMPLES OFF CACHE BOOL "")
    set(CMAKE_ROCKSDB_BUILD_SHARED OFF CACHE BOOL "")
    set(CMAKE_WITH_GFLAGS ON CACHE BOOL "")
    set(CMAKE_WITH_LIBURING OFF CACHE BOOL "")
    FetchContent_Populate(rocksdb)
endif()

FetchContent_MakeAvailable(rocksdb)
include_directories(${rocksdb_SOURCE_DIR}/include)
