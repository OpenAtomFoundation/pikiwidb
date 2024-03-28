# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

FILE(GLOB STORAGE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/storage/*.h"
        )
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
ADD_LIBRARY(storage ${STORAGE_SRC})

TARGET_INCLUDE_DIRECTORIES(storage
        PUBLIC ${CMAKE_SOURCE_DIR}/src
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include 
        PRIVATE ${rocksdb_SOURCE_DIR}/
        PRIVATE ${rocksdb_SOURCE_DIR}/include
        PRIVATE ${BRAFT_INCLUDE_DIR}
        PRIVATE ${BRPC_INCLUDE_DIR}
        PRIVATE ${PROTO_OUTPUT_DIR}
)

TARGET_LINK_LIBRARIES (storage 
  pstd 
  braft 
  brpc 
  ssl 
  crypto 
  zlib 
  leveldb 
  gflags 
  rocksdb 
  binlog_pb
  protobuf 
)

SET_TARGET_PROPERTIES(storage PROPERTIES LINKER_LANGUAGE CXX)
