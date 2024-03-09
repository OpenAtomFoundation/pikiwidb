# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE(ExternalProject)

SET(BRAFT_SOURCES_DIR ${THIRD_PARTY_PATH}/braft)
SET(BRAFT_INSTALL_DIR ${THIRD_PARTY_PATH}/install/braft)
SET(BRAFT_INCLUDE_DIR "${BRAFT_INSTALL_DIR}/include" CACHE PATH "braft include directory." FORCE)
SET(BRAFT_LIBRARIES "${BRAFT_INSTALL_DIR}/lib/libbraft.a" CACHE FILEPATH "braft library." FORCE)

SET(prefix_path "${THIRD_PARTY_PATH}/install/brpc|${CMAKE_CURRENT_BINARY_DIR}/_deps/gflags-build|${THIRD_PARTY_PATH}/install/protobuf|${THIRD_PARTY_PATH}/install/zlib|${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-build|${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-src")

ExternalProject_Add(
        extern_braft
        ${EXTERNAL_PROJECT_LOG_ARGS}
        DEPENDS brpc
        URL "https://github.com/baidu/braft/archive/v1.1.2.tar.gz"
        URL_HASH SHA256=bb3705f61874f8488e616ae38464efdec1a20610ddd6cd82468adc814488f14e
        PREFIX ${BRAFT_SOURCES_DIR}
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_INSTALL_PREFIX=${BRAFT_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR=${BRAFT_INSTALL_DIR}/lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${THIRD_PARTY_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${prefix_path}
        -DBRPC_WITH_GLOG=OFF
        -DWITH_DEBUG_SYMBOLS=OFF
        ${EXTERNAL_OPTIONAL_ARGS}
        LIST_SEPARATOR |
        CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${BRAFT_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR:PATH=${BRAFT_INSTALL_DIR}/lib
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=${THIRD_PARTY_BUILD_TYPE}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND $(MAKE) -j ${CPU_CORE} braft-static
        INSTALL_COMMAND mkdir -p ${BRAFT_INSTALL_DIR}/lib/ COMMAND cp ${BRAFT_SOURCES_DIR}/src/extern_braft/output/lib/libbraft.a ${BRAFT_LIBRARIES} COMMAND cp -r ${BRAFT_SOURCES_DIR}/src/extern_braft/output/include ${BRAFT_INCLUDE_DIR}/
)
ADD_DEPENDENCIES(extern_braft brpc)
ADD_LIBRARY(braft STATIC IMPORTED GLOBAL)
SET_PROPERTY(TARGET braft PROPERTY IMPORTED_LOCATION ${BRAFT_LIBRARIES})
ADD_DEPENDENCIES(braft extern_braft)