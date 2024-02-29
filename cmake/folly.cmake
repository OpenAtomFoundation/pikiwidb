# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

SET(DEPS_FETCH_PROXY "" CACHE STRING
    "a template URL to proxy the traffic for fetching dependencies, e.g. with DEPS_FETCH_PROXY = https://some-proxy/,
     https://example/some-dep.zip -> https://some-proxy/https://example/some-dep.zip")

CMAKE_HOST_SYSTEM_INFORMATION(RESULT CPU_CORE QUERY NUMBER_OF_LOGICAL_CORES)

IF (CMAKE_GENERATOR STREQUAL "Ninja")
  SET(MAKE_COMMAND make -j${CPU_CORE})
ELSE ()
  SET(MAKE_COMMAND $(MAKE) -j${CPU_CORE})
ENDIF ()

IF (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  CMAKE_POLICY(SET CMP0135 NEW) 
ENDIF ()

INCLUDE(cmake/utils.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/unwind.cmake)
INCLUDE(cmake/gflags.cmake)
INCLUDE(cmake/glog.cmake)
INCLUDE(cmake/double-conversion.cmake)
INCLUDE(cmake/fmt.cmake)

ADD_COMPILE_DEFINITIONS(FOLLY_NO_CONFIG)

FETCHCONTENT_DECLARE(pikiwidb-folly
  URL https://github.com/pikiwidb/folly/archive/v2023.10.16.00.zip
  URL_HASH SHA256=EB29DC13474E3979A0680F624FF5820FA7A4E9CE0110607669AE87D69CFC104D
  PATCH_COMMAND patch -p1 -s -E -i ${PROJECT_SOURCE_DIR}/cmake/patches/folly_coroutine.patch
)

FetchContent_MakeAvailableWithArgs(pikiwidb-folly)

TARGET_LINK_LIBRARIES(pikiwidb-folly pikiwidb-boost glog double-conversion fmt)
TARGET_INCLUDE_DIRECTORIES(pikiwidb-folly PUBLIC $<BUILD_INTERFACE:${pikiwidb-folly_SOURCE_DIR}>)
