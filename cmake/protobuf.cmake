# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

FETCHCONTENT_DECLARE(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG v25.1
)

SET(PROTOBUF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET(PROTOBUF_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
SET(PROTOBUF_INSTALL OFF CACHE BOOL "" FORCE)
# Enable ABSL_PROPAGATE_CXX_STD option
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailableWithArgs(protobuf protobuf_USE_EXTERNAL_GTEST=ON)
# FETCHCONTENT_MAKEAVAILABLE(protobuf)