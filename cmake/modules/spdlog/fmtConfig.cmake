# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

if(fmt_SOURCE_DIR)
  message(STATUS "Found fmt in ${fmt_SOURCE_DIR}")

  add_library(fmt::fmt ALIAS fmt)
endif()
