# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

include(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(spdlog
  gabime/spdlog v1.12.0
  SHA256=6174BF8885287422A6C6A0312EB8A30E8D22BCFCEE7C48A6D02D1835D7769232
)

FetchContent_MakeAvailableWithArgs(spdlog
  CMAKE_MODULE_PATH=${PROJECT_SOURCE_DIR}/cmake/modules/spdlog
  SPDLOG_FMT_EXTERNAL=ON
)
