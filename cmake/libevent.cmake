# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

# libevent
FETCHCONTENT_DECLARE(
        libevent
        GIT_REPOSITORY https://github.com/libevent/libevent.git
        GIT_TAG release-2.1.12-stable
)

SET(EVENT__DISABLE_THREAD_SUPPORT ON CACHE BOOL "" FORCE)
SET(EVENT__DISABLE_OPENSSL ON CACHE BOOL "" FORCE)
SET(EVENT__DISABLE_BENCHMz`ARK ON CACHE BOOL "" FORCE)
SET(EVENT__DISABLE_TESTS ON CACHE BOOL "" FORCE)
SET(EVENT__DISABLE_REGRESS ON CACHE BOOL "" FORCE)
SET(EVENT__DISABLE_SAMPLES ON CACHE BOOL "" FORCE)
SET(EVENT__LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
FETCHCONTENT_MAKEAVAILABLE(libevent)