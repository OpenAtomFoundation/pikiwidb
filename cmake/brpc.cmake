# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

if (CMAKE_VERSION VERSION_GREATER_EQUAL "2.8.12")
  cmake_policy(SET CMP0135 NEW)
endif()

include(cmake/gflags.cmake)
include(cmake/protobuf.cmake)
# include(cmake/openssl.cmake)
include(cmake/leveldb.cmake)

FETCHCONTENT_DECLARE(
        brpc
        GIT_REPOSITORY https://github.com/apache/brpc.git
        GIT_TAG 1.7.0
)

# SET(BRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# SET(BRPC_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
# # SET(BRPC_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_GetProperties(brpc)
if(NOT brpc_POPULATED)
	FetchContent_Populate(brpc)
	cmake_policy(SET CMP0069 NEW)
        SET(BRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        SET(BRPC_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
        set(BRPC_BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
	set(BRPC_BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_BRPC_LIB ON CACHE BOOL "Build brpc library" FORCE)
        set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules/brpc)
	add_subdirectory(${brpc_SOURCE_DIR} ${brpc_BINARY_DIR})
endif()

SET(BRPC_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/_deps/brpc-build/output/include)
SET(BRPC_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/brpc-build/output/lib)

add_custom_target(brpc DEPENDS protocolbuffers_protobuf gflags openssl leveldb)

# FetchContent_MakeAvailableWithArgs(brpc
#   CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake/modules/brpc
#   WITH_GFLAGS=ON
#   BUILD_TESTING=OFF
#   BUILD_STATIC_LIBS=ON
#   BUILD_SHARED_LIBS=ON  
# )
