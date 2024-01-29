//  Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <memory>
#include <string>

#include "pstd/lock_mgr.h"

#include "src/mutex.h"

namespace storage {

using LockMgr = pstd::lock::LockMgr;

}  //  namespace storage
