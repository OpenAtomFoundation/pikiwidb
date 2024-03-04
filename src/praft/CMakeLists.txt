# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
FILE(GLOB PRAFT_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
EXECUTE_PROCESS(
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} -I=${CMAKE_CURRENT_SOURCE_DIR} ${PRAFT_PROTO}
)

FILE(GLOB PRAFT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
ADD_LIBRARY(praft ${PRAFT_SRC})

TARGET_INCLUDE_DIRECTORIES(praft
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        PRIVATE ${PROJECT_SOURCE_DIR}/src/pstd
        PRIVATE ${PROJECT_SOURCE_DIR}/src/net
        PRIVATE ${PROJECT_SOURCE_DIR}/src/storage/include
        PRIVATE ${rocksdb_SOURCE_DIR}/
        PRIVATE ${rocksdb_SOURCE_DIR}/include
        PRIVATE ${BRAFT_INCLUDE_DIR}
        PRIVATE ${BRPC_INCLUDE_DIR}
        PRIVATE ${GFLAGS_INCLUDE_PATH}
        PRIVATE ${PROJECT_SOURCE_DIR}/src/praft
)

TARGET_LINK_LIBRARIES(praft net; dl; fmt; storage; pstd braft brpc ssl crypto zlib protobuf leveldb gflags rocksdb rt crypto dl z)

SET_TARGET_PROPERTIES(praft PROPERTIES LINKER_LANGUAGE CXX)
                      

