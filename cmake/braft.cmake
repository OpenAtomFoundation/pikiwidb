# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include(cmake/brpc.cmake)

FETCHCONTENT_DECLARE(
        braft
        GIT_REPOSITORY https://github.com/baidu/braft.git
        GIT_TAG v1.1.2
)

FetchContent_MakeAvailableWithArgs(braft
  CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake/modules/braft
  BUILD_TESTING=OFF
  BUILD_STATIC_LIBS=ON
  BUILD_SHARED_LIBS=ON
)