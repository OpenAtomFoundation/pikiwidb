# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

FetchContent_DeclareGitHubWithMirror(zstd
        facebook/zstd v1.5.5
        SHA256=c5c8daa1d40dabc51790c62a5b86af2b36dfc4e1a738ff10dc4a46ea4e68ee51
)

FetchContent_GetProperties(zstd)
if(NOT zstd_POPULATED)
    FetchContent_Populate(zstd)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(APPLE_FLAG "CFLAGS=-isysroot ${CMAKE_OSX_SYSROOT}")
    endif()

    add_custom_target(make_zstd COMMAND ${MAKE_COMMAND} CC=${CMAKE_C_COMPILER} ${APPLE_FLAG} libzstd.a
            WORKING_DIRECTORY ${zstd_SOURCE_DIR}/lib
            BYPRODUCTS ${zstd_SOURCE_DIR}/lib/libzstd.a
    )
endif()

add_library(zstd INTERFACE)
target_include_directories(zstd INTERFACE $<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib>)
target_link_libraries(zstd INTERFACE $<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib/libzstd.a>)
add_dependencies(zstd make_zstd)