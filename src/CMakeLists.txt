# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

AUX_SOURCE_DIRECTORY(. PIKIWIDB_SRC)

ADD_EXECUTABLE(pikiwidb ${PIKIWIDB_SRC})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

TARGET_INCLUDE_DIRECTORIES(pikiwidb PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/pstd
        ${PROJECT_SOURCE_DIR}/src/net
        ${PROJECT_SOURCE_DIR}/src/storage/include
        ${rocksdb_SOURCE_DIR}/
        ${rocksdb_SOURCE_DIR}/include
        PRIVATE ${BRAFT_INCLUDE_DIR}
        PRIVATE ${BRPC_INCLUDE_DIR}
)

find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(CSSERVICES_LIBRARY CoreServices)
find_library(CFN_LIBRARY CFNetwork)
find_library(SCY_LIBRARY Security)
find_library(COREGRAPHICS_LIBRARY CoreGraphics)
find_library(CORETEXT_LIBRARY CoreText)
find_library(Foundation_LIBRARY Foundation)

TARGET_LINK_LIBRARIES(pikiwidb net; dl; fmt; storage; rocksdb; braft brpc ssl crypto zlib protobuf leveldb gflags ${COREFOUNDATION_LIBRARY} ${CSSERVICES_LIBRARY} ${CFN_LIBRARY} ${SCY_LIBRARY} ${COREGRAPHICS_LIBRARY} ${CORETEXT_LIBRARY} ${Foundation_LIBRARY})
SET_TARGET_PROPERTIES(pikiwidb PROPERTIES LINKER_LANGUAGE CXX)
