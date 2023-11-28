# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

include(cmake/utils.cmake)

FetchContent_DeclareGitHubWithMirror(double-conversion
  google/double-conversion v3.3.0
  SHA256=4080014235f90854ffade6d1c423940b314bbca273a338235f049da296e47183
)
FetchContent_MakeAvailableWithArgs(double-conversion
        BUILD_TESTING=OFF
)
