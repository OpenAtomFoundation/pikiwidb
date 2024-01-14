# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include(cmake/brpc.cmake)

FETCHCONTENT_DECLARE(
        braft
        GIT_REPOSITORY https://github.com/baidu/braft.git
        GIT_TAG v1.1.2
)

# SET(BRAFT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# SET(BRAFT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
# SET(BRAFT_INSTALL OFF CACHE BOOL "" FORCE)


# FetchContent_MakeAvailableWithArgs(braft)
# target_link_libraries(pikiwidb-braft brpc gflags_static protobuf leveldb)

# FetchContent_GetProperties(braft)
# if(NOT braft_POPULATED)
# 	FetchContent_Populate(braft)
# 	cmake_policy(SET CMP0069 NEW)
#         SET(BRAFT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#         SET(BRAFT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
#         set(BRAFT_BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
# 	set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
# 	set(BRAFT_BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
# 	set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
# 	set(BUILD_BRAFT_LIB ON CACHE BOOL "Build braft library" FORCE)
# 	add_subdirectory(${braft_SOURCE_DIR} ${braft_BINARY_DIR})
# 	add_custom_target(braft DEPENDS brpc)
# endif()

FetchContent_MakeAvailableWithArgs(braft
  CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake/modules/braft
  BUILD_TESTING=OFF
  BUILD_STATIC_LIBS=ON
  BUILD_SHARED_LIBS=ON
)