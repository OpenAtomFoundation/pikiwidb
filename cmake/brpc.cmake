# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE(ExternalProject)

SET(BRPC_SOURCES_DIR ${THIRD_PARTY_PATH}/brpc)
SET(BRPC_INSTALL_DIR ${THIRD_PARTY_PATH}/install/brpc)
SET(BRPC_INCLUDE_DIR "${BRPC_INSTALL_DIR}/include" CACHE PATH "brpc include directory." FORCE)
SET(BRPC_LIBRARIES "${BRPC_INSTALL_DIR}/lib/libbrpc.a" CACHE FILEPATH "brpc library." FORCE)

# Reference https://stackoverflow.com/questions/45414507/pass-a-list-of-prefix-paths-to-externalproject-add-in-cmake-args
SET(prefix_path "${CMAKE_CURRENT_BINARY_DIR}/_deps/gflags-build|${THIRD_PARTY_PATH}/install/protobuf|${THIRD_PARTY_PATH}/install/zlib|${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-build|${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-src")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
# If minimal .a is need, you can set  WITH_DEBUG_SYMBOLS=OFF
EXTERNALPROJECT_ADD( 
        extern_brpc
        ${EXTERNAL_PROJECT_LOG_ARGS}
        DEPENDS ssl crypto zlib protobuf leveldb gflags
        URL "https://github.com/apache/brpc/archive/1.3.0.tar.gz"
        URL_HASH SHA256=b9d638b76725552ed11178c650d7fc95e30f252db7972a93dc309a0698c7d2b8
        PREFIX ${BRPC_SOURCES_DIR}
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_INSTALL_PREFIX=${BRPC_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR=${BRPC_INSTALL_DIR}/lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=${THIRD_PARTY_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${prefix_path}
        -DWITH_GLOG=OFF
        -DDOWNLOAD_GTEST=OFF
        ${EXTERNAL_OPTIONAL_ARGS}
        LIST_SEPARATOR |
        CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${BRPC_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR:PATH=${BRPC_INSTALL_DIR}/lib
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=${THIRD_PARTY_BUILD_TYPE}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND $(MAKE) -j ${CPU_CORE} brpc-static
        INSTALL_COMMAND mkdir -p ${BRPC_INSTALL_DIR}/lib/ COMMAND cp ${BRPC_SOURCES_DIR}/src/extern_brpc/output/lib/libbrpc.a ${BRPC_LIBRARIES} COMMAND cp -r ${BRPC_SOURCES_DIR}/src/extern_brpc/output/include ${BRPC_INCLUDE_DIR}/
)
ADD_DEPENDENCIES(extern_brpc ssl crypto zlib protobuf leveldb gflags)
ADD_LIBRARY(brpc STATIC IMPORTED GLOBAL)
SET_PROPERTY(TARGET brpc PROPERTY IMPORTED_LOCATION ${BRPC_LIBRARIES})
ADD_DEPENDENCIES(brpc extern_brpc)
