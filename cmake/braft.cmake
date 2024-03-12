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
				GIT_REPOSITORY https://github.com/baidu/braft.git
        GIT_TAG master
        UPDATE_COMMAND ""
				CMAKE_ARGS
					-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
					-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
					-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
					-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
					${EXTERNAL_OPTIONAL_ARGS}
        	LIST_SEPARATOR |
        CMAKE_CACHE_ARGS 
					-DCMAKE_PREFIX_PATH:PATH=${prefix_path}
					-DCMAKE_INSTALL_PREFIX:PATH=${BRAFT_INSTALL_DIR}
					-DCMAKE_BUILD_TYPE:STRING=${THIRD_PARTY_BUILD_TYPE}  # Set build type
					-DBRPC_WITH_GLOG:BOOL=OFF
					-DBUILD_SHARED_LIBS:BOOL=OFF
        BUILD_IN_SOURCE 1
        BUILD_COMMAND $(MAKE) -j ${CPU_CORE} braft-static
)
ADD_DEPENDENCIES(extern_braft brpc)
ADD_LIBRARY(braft STATIC IMPORTED GLOBAL)
SET_PROPERTY(TARGET braft PROPERTY IMPORTED_LOCATION ${BRAFT_LIBRARIES})
ADD_DEPENDENCIES(braft extern_braft)
