# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

if(gflags_SOURCE_DIR)
  message(STATUS "Found gflags in ${gflags_SOURCE_DIR}")

#   add_library(gflags_static::gflags_static ALIAS gflags_static)
#   install(TARGETS gflags_static EXPORT glog-targets)
endif()
