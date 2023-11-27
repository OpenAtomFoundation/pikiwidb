# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

include_guard()

FetchContent_DeclareGitHubWithMirror(zlib
        madler/zlib v1.3
        SHA256=e6ee0c09dccf864ec23f2df075401cc7c68a67a8a633ff182e7abcb7c673356e
)

FetchContent_MakeAvailableWithArgs(zlib)
