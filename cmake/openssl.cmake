# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

SET(OPENSSL_FETCH_INFO
        URL https://www.openssl.org/source/openssl-1.1.1h.tar.gz
        URL_HASH SHA256=5c9ca8774bd7b03e5784f26ae9e9e6d749c9da2438545077e6b3d755a06595d9
        )
SET(OPENSSL_USE_STATIC_LIBS ON)

FETCHCONTENT_DECLARE(
        openssl
        GIT_REPOSITORY https://github.com/jc-lab/openssl-cmake.git
        GIT_TAG        39af37e0964d71c516da5b1836849dd0a03df7a4 # Change to the latest commit ID
)

FETCHCONTENT_GETPROPERTIES(openssl)
IF (NOT openssl_POPULATED)
        FETCHCONTENT_POPULATE(openssl)
        ADD_SUBDIRECTORY(${openssl_SOURCE_DIR} ${openssl_BINARY_DIR})
ENDIF ()

SET(OPENSSL_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/openssl_src-src/include)
SET(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/openssl_src-src)
SET(OPENSSL_CRYPTO_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/_deps/openssl_src-src)