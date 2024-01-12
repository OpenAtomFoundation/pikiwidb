# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

include(cmake/utils.cmake)

# FetchContent_DeclareGitHubWithMirror(gflags
#   gflags/gflags v2.2.2
#   SHA256=19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5
# )

# FetchContent_MakeAvailableWithArgs(gflags
#   GFLAGS_NAMESPACE=gflags
#   BUILD_STATIC_LIBS=ON
#   BUILD_SHARED_LIBS=OFF
#   BUILD_gflags_LIB=ON
#   BUILD_gflags_nothreads_LIB=OFF
#   BUILD_TESTING=OFF
# )

# find_package(Threads REQUIRED)

# target_link_libraries(gflags_static Threads::Threads)


FetchContent_Declare(gflags
	GIT_REPOSITORY	https://github.com/gflags/gflags.git
	GIT_TAG			master
)

FetchContent_GetProperties(gflags)
if(NOT gflags_POPULATED)
	FetchContent_Populate(gflags)
	cmake_policy(SET CMP0069 NEW)
  set(GFLAGS_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
	set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
	set(BUILD_gflags_LIB ON CACHE BOOL "Build gflags library" FORCE)
	add_subdirectory(${gflags_SOURCE_DIR} ${gflags_BINARY_DIR})
endif()

# FetchContent_Declare(gflags
#   URL      https://github.com/gflags/gflags/archive/v2.2.2.zip
#   URL_HASH SHA256=19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5
# )
# FetchContent_MakeAvailable(gflags)