/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "lock_mgr.h"
#include "noncopyable.h"
#include "rocksdb/slice.h"

namespace pstd::lock {

using Slice = rocksdb::Slice;

//* 实现了一个 RAII 的锁对象。
class ScopeRecordLock final : public pstd::noncopyable {
 public:
  // 构造对象的时候传入一个锁管理器以及要上锁的 key ，对象产生即使用锁管理器对 key 上锁。
  ScopeRecordLock(const std::shared_ptr<LockMgr>& lock_mgr, const Slice& key) : lock_mgr_(lock_mgr), key_(key) {
    lock_mgr_->TryLock(key_.ToString());
  }
  // 析构时使用锁管理器对 key 解锁。
  ~ScopeRecordLock() { lock_mgr_->UnLock(key_.ToString()); }

 private:
  std::shared_ptr<LockMgr> const lock_mgr_;
  Slice key_;
};

class MultiScopeRecordLock final : public pstd::noncopyable {
 public:
  MultiScopeRecordLock(const std::shared_ptr<LockMgr>& lock_mgr, const std::vector<std::string>& keys);
  ~MultiScopeRecordLock();

 private:
  std::shared_ptr<LockMgr> const lock_mgr_;
  std::vector<std::string> keys_;
};

class MultiRecordLock : public noncopyable {
 public:
  explicit MultiRecordLock(const std::shared_ptr<LockMgr>& lock_mgr) : lock_mgr_(lock_mgr) {}
  ~MultiRecordLock() = default;

  void Lock(const std::vector<std::string>& keys);
  void Unlock(const std::vector<std::string>& keys);

 private:
  std::shared_ptr<LockMgr> const lock_mgr_;
};

}  // namespace pstd::lock
