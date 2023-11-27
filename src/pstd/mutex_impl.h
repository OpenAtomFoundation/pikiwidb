/*
* Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "mutex.h"

#include <memory>

namespace pstd {
namespace lock {
// Default implementation of MutexFactory.
class MutexFactoryImpl : public MutexFactory {
public:
  std::shared_ptr<Mutex> AllocateMutex() override;
  std::shared_ptr<CondVar> AllocateCondVar() override;
};
}  //  namespace lock
}  //  namespace pstd
