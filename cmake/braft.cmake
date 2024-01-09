# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

FETCHCONTENT_DECLARE(
        pikiwidb-braft
        GIT_REPOSITORY https://github.com/baidu/braft.git
        GIT_TAG v1.1.2
)

SET(BRAFT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(BRAFT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
SET(BRAFT_INSTALL OFF CACHE BOOL "" FORCE)
FETCHCONTENT_MAKEAVAILABLE(pikiwidb-braft)

# link_directories(${BRPC_DIR})
target_link_libraries(pikiwidb-braft brpc gflags_static protobuf leveldb)