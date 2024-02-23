# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.3.3
)

FetchContent_MakeAvailableWithArgs(rocksdb
        BUILD_TYPE=OFF
        WITH_TESTS=OFF
        WITH_BENCHMARK=OFF
        WITH_BENCHMARK_TOOLS=OFF
        WITH_TOOLS=OFF
        WITH_CORE_TOOLS=OFF
        WITH_TRACE_TOOLS=OFF
        WITH_EXAMPLES=OFF
        ROCKSDB_BUILD_SHARED=OFF
        WITH_LIBURING=OFF
        WITH_LZ4=OFF
        WITH_SNAPPY=OFF
        WITH_ZLIB=ON
        WITH_ZSTD=OFF
        WITH_GFLAGS=OFF
        USE_RTTI=ON
)
