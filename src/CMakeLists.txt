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
        PRIVATE ${GFLAGS_INCLUDE_PATH}
        PRIVATE ${PROJECT_SOURCE_DIR}/src/praft
        )

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
    FIND_LIBRARY(CSSERVICES_LIBRARY CoreServices)
    FIND_LIBRARY(CFN_LIBRARY CFNetwork)
    FIND_LIBRARY(SCY_LIBRARY Security)
    FIND_LIBRARY(COREGRAPHICS_LIBRARY CoreGraphics)
    FIND_LIBRARY(CORETEXT_LIBRARY CoreText)
    FIND_LIBRARY(Foundation_LIBRARY Foundation)
    LIST(APPEND MAC_LIBRARY ${COREFOUNDATION_LIBRARY} ${CSSERVICES_LIBRARY} ${CFN_LIBRARY} ${SCY_LIBRARY} ${COREGRAPHICS_LIBRARY} ${CORETEXT_LIBRARY} ${Foundation_LIBRARY})
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(MAC_LIBRARY "")
ELSE ()
    MESSAGE(FATAL_ERROR "only support linux or macOS")
ENDIF ()

TARGET_LINK_LIBRARIES(pikiwidb net; dl; fmt; storage; rocksdb; pstd braft brpc ssl crypto zlib protobuf leveldb gflags rt crypto dl z praft "${MAC_LIBRARY}")
SET_TARGET_PROPERTIES(pikiwidb PROPERTIES LINKER_LANGUAGE CXX)