# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

set(DEPS_FETCH_PROXY "" CACHE STRING
    "a template URL to proxy the traffic for fetching dependencies, e.g. with DEPS_FETCH_PROXY = https://some-proxy/,
     https://example/some-dep.zip -> https://some-proxy/https://example/some-dep.zip")

cmake_host_system_information(RESULT CPU_CORE QUERY NUMBER_OF_LOGICAL_CORES)

if(CMAKE_GENERATOR STREQUAL "Ninja")
  set(MAKE_COMMAND make -j${CPU_CORE})
else()
  set(MAKE_COMMAND $(MAKE) -j${CPU_CORE})
endif()

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(cmake/utils.cmake)
include(cmake/boost.cmake)
include(cmake/unwind.cmake)
include(cmake/gflags.cmake)
include(cmake/glog.cmake)
include(cmake/double-conversion.cmake)
include(cmake/fmt.cmake)

add_compile_definitions(FOLLY_NO_CONFIG)

FetchContent_Declare(pikiwidb-folly
  URL https://github.com/pikiwidb/folly/archive/v2023.10.16.00.zip
  URL_HASH SHA256=EB29DC13474E3979A0680F624FF5820FA7A4E9CE0110607669AE87D69CFC104D
  PATCH_COMMAND patch -p1 -s -E -i ${PROJECT_SOURCE_DIR}/cmake/patches/folly_coroutine.patch
)

FetchContent_MakeAvailableWithArgs(pikiwidb-folly)

target_link_libraries(pikiwidb-folly pikiwidb-boost glog double-conversion fmt)
target_include_directories(pikiwidb-folly PUBLIC $<BUILD_INTERFACE:${pikiwidb-folly_SOURCE_DIR}>)
