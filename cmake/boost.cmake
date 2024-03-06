# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

INCLUDE_GUARD()

INCLUDE(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(pikiwidb-boost
  pikiwidb/boost boost-1.83.0
  SHA256=A3B453E3D5FD39E6A4C733C31548512A1E74B7328D4C358FAC562930A0E6E5B4
)

FetchContent_MakeAvailableWithArgs(pikiwidb-boost)
