//  Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef TYPE_ITERATOR_H_
#define TYPE_ITERATOR_H_

#include <memory>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"

#include "src/base_data_key_format.h"
#include "src/base_key_format.h"
#include "src/base_meta_value_format.h"
#include "src/debug.h"
#include "src/lists_meta_value_format.h"
#include "src/mutex.h"
#include "src/strings_value_format.h"
#include "storage/storage_define.h"
#include "storage/util.h"
#include "util/heap.h"

namespace storage {
using ColumnFamilyHandle = rocksdb::ColumnFamilyHandle;
using Comparator = rocksdb::Comparator;

enum Direction { kForward, kReverse };

class TypeIterator {
 public:
  TypeIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle) {
    raw_iter_.reset(db->NewIterator(options, handle));
  }

  virtual ~TypeIterator() {}

  virtual void Seek(const std::string& start_key) {
    raw_iter_->Seek(Slice(start_key));
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Next();
    }
  }

  void SeekToFirst() {
    raw_iter_->SeekToFirst();
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Next();
    }
  }

  void SeekToLast() {
    raw_iter_->SeekToLast();
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Prev();
    }
  }

  virtual void SeekForPrev(const std::string& start_key) {
    raw_iter_->SeekForPrev(Slice(start_key));
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Prev();
    }
  }

  void Next() {
    raw_iter_->Next();
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Next();
    }
  }

  void Prev() {
    raw_iter_->Prev();
    while (raw_iter_->Valid() && ShouldSkip()) {
      raw_iter_->Prev();
    }
  }

  virtual bool ShouldSkip() { return false; }

  virtual std::string Key() const { return user_key_; }

  virtual std::string Value() const { return user_value_; }

  virtual bool Valid() { return raw_iter_->Valid(); }

  virtual Status status() { return raw_iter_->status(); }

 protected:
  std::unique_ptr<rocksdb::Iterator> raw_iter_;
  std::string user_key_;
  std::string user_value_;
  Direction direction_ = kForward;
};

class StringsIterator : public TypeIterator {
 public:
  StringsIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle,
                  const std::string& pattern)
      : TypeIterator(options, db, handle), pattern_(pattern) {}
  ~StringsIterator() {}

  bool ShouldSkip() override {
    ParsedStringsValue parsed_value(raw_iter_->value());
    if (parsed_value.IsStale()) {
      return true;
    }

    ParsedBaseKey parsed_key(raw_iter_->key().ToString());
    if (StringMatch(pattern_.data(), pattern_.size(), parsed_key.Key().data(), parsed_key.Key().size(), 0) == 0) {
      return true;
    }

    user_key_ = parsed_key.Key().ToString();
    user_value_ = parsed_value.UserValue().ToString();
    return false;
  }

 private:
  std::string pattern_;
};

class HashesIterator : public TypeIterator {
 public:
  HashesIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle,
                 const std::string& pattern)
      : TypeIterator(options, db, handle), pattern_(pattern) {}
  ~HashesIterator() {}

  bool ShouldSkip() override {
    ParsedHashesMetaValue parsed_meta_value(raw_iter_->value());
    if (parsed_meta_value.IsStale() || parsed_meta_value.Count() == 0) {
      return true;
    }

    ParsedBaseMetaKey parsed_key(raw_iter_->key().ToString());
    if (StringMatch(pattern_.data(), pattern_.size(), parsed_key.Key().data(), parsed_key.Key().size(), 0) == 0) {
      return true;
    }
    user_key_ = parsed_key.Key().ToString();
    user_value_ = parsed_meta_value.UserValue().ToString();
    return false;
  }

 private:
  std::string pattern_;
};

class ListsIterator : public TypeIterator {
 public:
  ListsIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle,
                const std::string& pattern)
      : TypeIterator(options, db, handle), pattern_(pattern) {}
  ~ListsIterator() {}

  bool ShouldSkip() override {
    ParsedListsMetaValue parsed_meta_value(raw_iter_->value());
    if (parsed_meta_value.IsStale() || parsed_meta_value.Count() == 0) {
      return true;
    }

    ParsedBaseMetaKey parsed_key(raw_iter_->key().ToString());
    if (StringMatch(pattern_.data(), pattern_.size(), parsed_key.Key().data(), parsed_key.Key().size(), 0) == 0) {
      return true;
    }
    user_key_ = parsed_key.Key().ToString();
    user_value_ = parsed_meta_value.UserValue().ToString();
    return false;
  }

 private:
  std::string pattern_;
};

class SetsIterator : public TypeIterator {
 public:
  SetsIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle,
               const std::string& pattern)
      : TypeIterator(options, db, handle), pattern_(pattern) {}
  ~SetsIterator() {}

  bool ShouldSkip() override {
    ParsedSetsMetaValue parsed_meta_value(raw_iter_->value());
    if (parsed_meta_value.IsStale() || parsed_meta_value.Count() == 0) {
      return true;
    }

    ParsedBaseMetaKey parsed_key(raw_iter_->key().ToString());
    if (StringMatch(pattern_.data(), pattern_.size(), parsed_key.Key().data(), parsed_key.Key().size(), 0) == 0) {
      return true;
    }
    user_key_ = parsed_key.Key().ToString();
    user_value_ = parsed_meta_value.UserValue().ToString();
    return false;
  }

 private:
  std::string pattern_;
};

class ZsetsIterator : public TypeIterator {
 public:
  ZsetsIterator(const rocksdb::ReadOptions& options, rocksdb::DB* db, ColumnFamilyHandle* handle,
                const std::string& pattern)
      : TypeIterator(options, db, handle), pattern_(pattern) {}
  ~ZsetsIterator() {}

  bool ShouldSkip() override {
    ParsedZSetsMetaValue parsed_meta_value(raw_iter_->value());
    if (parsed_meta_value.IsStale() || parsed_meta_value.Count() == 0) {
      return true;
    }

    ParsedBaseMetaKey parsed_key(raw_iter_->key().ToString());
    if (StringMatch(pattern_.data(), pattern_.size(), parsed_key.Key().data(), parsed_key.Key().size(), 0) == 0) {
      return true;
    }
    user_key_ = parsed_key.Key().ToString();
    user_value_ = parsed_meta_value.UserValue().ToString();
    return false;
  }

 private:
  std::string pattern_;
};

using IterSptr = std::shared_ptr<TypeIterator>;

class MinMergeComparator {
 public:
  MinMergeComparator() = default;
  bool operator()(IterSptr a, IterSptr b) {
    int a_len = a->Key().size();
    int b_len = b->Key().size();
    return a->Key().compare(b->Key()) > 0;
  }
};

class MaxMergeComparator {
 public:
  MaxMergeComparator() = default;
  bool operator()(IterSptr a, IterSptr b) {
    int a_len = a->Key().size();
    int b_len = b->Key().size();
    return a->Key().compare(b->Key()) < 0;
  }
};

using MergerMinIterHeap = rocksdb::BinaryHeap<IterSptr, MinMergeComparator>;
using MergerMaxIterHeap = rocksdb::BinaryHeap<IterSptr, MaxMergeComparator>;

class MergingIterator {
 public:
  MergingIterator(const std::vector<IterSptr>& children) : current_(nullptr), direction_(kForward) {
    std::copy(children.begin(), children.end(), std::back_inserter(children_));
    for (const auto& child : children_) {
      if (child->Valid()) {
        min_heap_.push(child);
      }
    }
    current_ = min_heap_.empty() ? nullptr : min_heap_.top();
  }

  ~MergingIterator() {}

  bool Valid() const { return current_ != nullptr; }

  Status status() const {
    Status status;
    for (const auto& child : children_) {
      status = child->status();
      if (!status.ok()) {
        break;
      }
    }
    return status;
  }

  bool IsFinished(const std::string& prefix) {
    if (Valid() && (Key().compare(prefix) <= 0 || Key().substr(0, prefix.size()) == prefix)) {
      return false;
    }
    return true;
  }

  void SeekToFirst() {
    min_heap_.clear();
    max_heap_.clear();
    for (auto& child : children_) {
      child->SeekToFirst();
      if (child->Valid()) {
        min_heap_.push(child);
      }
    }
    direction_ = kForward;
    current_ = min_heap_.empty() ? nullptr : min_heap_.top();
  }

  void SeekToLast() {
    min_heap_.clear();
    max_heap_.clear();
    for (auto& child : children_) {
      child->SeekToLast();
      if (child->Valid()) {
        max_heap_.push(child);
      }
    }
    direction_ = kReverse;
    current_ = max_heap_.empty() ? nullptr : max_heap_.top();
  }

  void Seek(const std::string& target) {
    min_heap_.clear();
    max_heap_.clear();
    for (auto& child : children_) {
      child->Seek(target);
      if (child->Valid()) {
        min_heap_.push(child);
      }
    }
    direction_ = kForward;
    current_ = min_heap_.empty() ? nullptr : min_heap_.top();
  }

  void SeekForPrev(const std::string& start_key) {
    min_heap_.clear();
    max_heap_.clear();
    for (auto& child : children_) {
      child->SeekForPrev(start_key);
      if (child->Valid()) {
        max_heap_.push(child);
      }
    }
    direction_ = kReverse;
    current_ = max_heap_.empty() ? nullptr : max_heap_.top();
  }

  void Next() {
    assert(direction_ == kForward);
    current_->Next();
    if (current_->Valid()) {
      min_heap_.replace_top(current_);
    } else {
      min_heap_.pop();
    }
    current_ = min_heap_.empty() ? nullptr : min_heap_.top();
  }

  void Prev() {
    assert(direction_ == kReverse);
    current_->Prev();
    if (current_->Valid()) {
      max_heap_.replace_top(current_);
    } else {
      max_heap_.pop();
    }
    current_ = max_heap_.empty() ? nullptr : max_heap_.top();
  }

  std::string Key() { return current_->Key(); }

  std::string Value() { return current_->Value(); }

  Status status() {
    Status s;
    for (const auto& child : children_) {
      s = child->status();
      if (!s.ok()) {
        break;
      }
    }
    return s;
  }

  bool Valid() { return current_ != nullptr; }

 private:
  MergerMinIterHeap min_heap_;
  MergerMaxIterHeap max_heap_;
  std::vector<IterSptr> children_;
  IterSptr current_;
  Direction direction_;
};

}  // end namespace storage

#endif
