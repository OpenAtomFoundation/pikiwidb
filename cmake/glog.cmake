# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

INCLUDE(cmake/utils.cmake)

FETCHCONTENT_DECLARE(glog
  URL https://github.com/google/glog/archive/v0.6.0.zip
  URL_HASH SHA256=122fb6b712808ef43fbf80f75c52a21c9760683dae470154f02bddfc61135022
  PATCH_COMMAND patch -p1 -s -E -i ${PROJECT_SOURCE_DIR}/cmake/patches/glog_demangle.patch
)

FetchContent_MakeAvailableWithArgs(glog
  CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake/modules/glog
  WITH_GFLAGS=ON
  BUILD_TESTING=OFF
  BUILD_SHARED_LIBS=OFF
  WITH_UNWIND=ON
)

SET(GLOG_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/_deps/glog-src/src CACHE BOOL "" FORCE)
IF (CMAKE_BUILD_TYPE STREQUAL "Release")
  SET(GLOG_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/glog-build/libglog.a CACHE BOOL "" FORCE)
ELSEIF (CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(GLOG_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/glog-build/libglogd.a CACHE BOOL "" FORCE)
ENDIF()