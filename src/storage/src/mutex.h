//  Copyright (c) 2017-present, Qihoo, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <memory>

#include "rocksdb/status.h"

#include "pstd/mutex.h"

namespace storage {

using Status = rocksdb::Status;

using Mutex = pstd::lock::Mutex;
using CondVar = pstd::lock::CondVar;
using MutexFactory = pstd::lock::MutexFactory;

}  // namespace storage
