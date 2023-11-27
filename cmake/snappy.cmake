# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

FetchContent_DeclareGitHubWithMirror(snappy
        google/snappy 1.1.10
        SHA256=3c6f7b07f92120ebbba5f7742f2cc2386fd46c53f5730322b7b90d4afc126fca
)

FetchContent_MakeAvailableWithArgs(snappy
        SNAPPY_BUILD_TESTS=OFF
        SNAPPY_BUILD_BENCHMARKS=OFF
        BUILD_STATIC_LIBS=ON
        BUILD_SHARED_LIBS=OFF
)
