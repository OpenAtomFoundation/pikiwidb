# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

IF (unwind_SOURCE_DIR)
  MESSAGE(STATUS "Found unwind in ${unwind_SOURCE_DIR}")

  ADD_LIBRARY(unwind::unwind ALIAS unwind)
  INSTALL(TARGETS unwind EXPORT glog-targets)
ENDIF()
