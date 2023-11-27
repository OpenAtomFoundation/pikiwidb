# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

FetchContent_DeclareGitHubWithMirror(lz4
        lz4/lz4 v1.9.4
        SHA256=37e63d56fb9cbe2e430c7f737a404cd4b98637b05e1467459d5c8fe1a4364cc3
)

FetchContent_GetProperties(lz4)
if(NOT lz4_POPULATED)
    FetchContent_Populate(lz4)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(APPLE_FLAG "CFLAGS=-isysroot ${CMAKE_OSX_SYSROOT}")
    endif()

    add_custom_target(make_lz4 COMMAND ${MAKE_COMMAND} CC=${CMAKE_C_COMPILER} ${APPLE_FLAG} liblz4.a
            WORKING_DIRECTORY ${lz4_SOURCE_DIR}/lib
            BYPRODUCTS ${lz4_SOURCE_DIR}/lib/liblz4.a
    )
endif()

add_library(lz4 INTERFACE)
target_include_directories(lz4 INTERFACE $<BUILD_INTERFACE:${lz4_SOURCE_DIR}/lib>)
target_link_libraries(lz4 INTERFACE $<BUILD_INTERFACE:${lz4_SOURCE_DIR}/lib/liblz4.a>)
add_dependencies(lz4 make_lz4)