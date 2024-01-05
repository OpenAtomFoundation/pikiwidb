# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

cmake_minimum_required(VERSION 3.18)

include(GoogleTest)
set(CMAKE_CXX_STANDARD 20)

aux_source_directory(.. DIR_SRCS)

file(GLOB_RECURSE PSTD_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

foreach (pstd_test_source ${PSTD_TEST_SOURCE})
    get_filename_component(pstd_test_filename ${pstd_test_source} NAME)
    string(REPLACE ".cc" "" pstd_test_name ${pstd_test_filename})

#    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
    add_executable(${pstd_test_name} ${pstd_test_source})
    target_include_directories(${pstd_test_name}
            PUBLIC ${PROJECT_SOURCE_DIR}/src
            )

    add_dependencies(${pstd_test_name} pstd gtest)
    target_link_libraries(${pstd_test_name}
            PUBLIC pstd
            PUBLIC gtest
            )
    gtest_discover_tests(${pstd_test_name})
endforeach ()
