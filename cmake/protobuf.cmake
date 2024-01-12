# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

FETCHCONTENT_DECLARE(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG v25.1
)
# set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# SET(protobuf_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
# SET(protobuf_INSTALL ON CACHE BOOL "" FORCE)
# set(protobuf_BUILD_SHARED_LIBS_DEFAULT ON CACHE BOOL "Build shared libraries" FORCE)
# set(protobuf_BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
# Enable ABSL_PROPAGATE_CXX_STD option
# set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailableWithArgs(protobuf)

FetchContent_GetProperties(protobuf)
if(NOT protobuf_POPULATED)
	FetchContent_Populate(protobuf)
	cmake_policy(SET CMP0069 NEW)
        set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
        set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_SHARED_LIBS_DEFAULT ON CACHE BOOL "Build shared libraries" FORCE)
        set(protobuf_BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
	set(protobuf_BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_protobuf_LIB ON CACHE BOOL "Build protobuf library" FORCE)
	add_subdirectory(${protobuf_SOURCE_DIR} ${protobuf_BINARY_DIR})
endif()