# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

AUX_SOURCE_DIRECTORY(. NET_SRC)
AUX_SOURCE_DIRECTORY(./lzf NET_SRC)
LIST(FILTER NET_SRC EXCLUDE REGEX ".*_test.cc")
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
ADD_LIBRARY(net ${NET_SRC})

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/pstd)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/net)

TARGET_LINK_LIBRARIES(net; pstd event_extra event_core llhttp::llhttp)

SET_TARGET_PROPERTIES(net PROPERTIES LINKER_LANGUAGE CXX)
