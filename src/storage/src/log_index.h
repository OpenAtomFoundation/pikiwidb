/*
 * Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <optional>
#include <shared_mutex>
#include <string_view>

#include "rocksdb/db.h"
#include "rocksdb/listener.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"

#include "storage/storage_define.h"

namespace storage {

using rocksdb::SequenceNumber;
class Redis;

class LogIndexAndSequencePair {
 public:
  LogIndexAndSequencePair(LogIndex applied_log_index, SequenceNumber seqno)
      : applied_log_index_(applied_log_index), seqno_(seqno) {}

  void SetAppliedLogIndex(LogIndex applied_log_index) { applied_log_index_ = applied_log_index; }
  void SetSequenceNumber(SequenceNumber seqno) { seqno_ = seqno; }

  LogIndex GetAppliedLogIndex() const { return applied_log_index_; }
  SequenceNumber GetSequenceNumber() const { return seqno_; }

 private:
  LogIndex applied_log_index_ = 0;
  SequenceNumber seqno_ = 0;
};

struct LogIndexSeqnoPair {
  std::atomic<LogIndex> log_index = 0;
  std::atomic<SequenceNumber> seqno = 0;

  LogIndex GetLogIndex() const { return log_index.load(); }

  SequenceNumber GetSequenceNumber() const { return seqno.load(); }

  void SetLogIndexSeqnoPair(LogIndex l, SequenceNumber s) {
    log_index.store(l);
    seqno.store(s);
  }

  LogIndexSeqnoPair() = default;
  
  // Copy constructor
  LogIndexSeqnoPair(const LogIndexSeqnoPair& other) 
    : log_index(other.log_index.load()), seqno(other.seqno.load()) {}
  
  // Copy assignment operator
  LogIndexSeqnoPair& operator=(const LogIndexSeqnoPair& other) {
    if (this != &other) {
      log_index.store(other.log_index.load());
      seqno.store(other.seqno.load());
    }
    return *this;
  }

  bool operator==(const LogIndexSeqnoPair &other) const { return seqno.load() == other.seqno.load(); }

  bool operator<=(const LogIndexSeqnoPair &other) const { return seqno.load() <= other.seqno.load(); }

  bool operator>=(const LogIndexSeqnoPair &other) const { return seqno.load() >= other.seqno.load(); }

  bool operator<(const LogIndexSeqnoPair &other) const { return seqno.load() < other.seqno.load(); }
};

class LogIndexOfColumnFamilies {
  struct LogIndexPair {
    LogIndexSeqnoPair applied_index;  // newest record in memtable.
    LogIndexSeqnoPair flushed_index;  // newest record in sst file.
  };

  struct SmallestIndexRes {
    int smallest_applied_log_index_cf = -1;
    LogIndex smallest_applied_log_index = std::numeric_limits<LogIndex>::max();

    int smallest_flushed_log_index_cf = -1;
    LogIndex smallest_flushed_log_index = std::numeric_limits<LogIndex>::max();
    SequenceNumber smallest_flushed_seqno = std::numeric_limits<SequenceNumber>::max();
  };

 public:
  // Read the largest log index of each column family from all sst files
  rocksdb::Status Init(Redis *db);

  SmallestIndexRes GetSmallestLogIndex(int flush_cf) const;

  void SetFlushedLogIndex(size_t cf_id, LogIndex log_index, SequenceNumber seqno) {
    if (cf_id >= cf_.size()) return;
    cf_[cf_id].flushed_index.log_index.store(std::max(cf_[cf_id].flushed_index.log_index.load(), log_index));
    cf_[cf_id].flushed_index.seqno.store(std::max(cf_[cf_id].flushed_index.seqno.load(), seqno));
  }

  void SetFlushedLogIndexGlobal(LogIndex log_index, SequenceNumber seqno) {
    SetLastFlushIndex(log_index, seqno);
    for (size_t i = 0; i < cf_.size(); i++) {
      if (cf_[i].flushed_index <= last_flush_index_) {
        auto flush_log_index = std::max(cf_[i].flushed_index.GetLogIndex(), last_flush_index_.GetLogIndex());
        auto flush_sequence_number =
            std::max(cf_[i].flushed_index.GetSequenceNumber(), last_flush_index_.GetSequenceNumber());
        cf_[i].flushed_index.SetLogIndexSeqnoPair(flush_log_index, flush_sequence_number);
      }
    }
  }

  bool IsApplied(size_t cf_id, LogIndex cur_log_index) const {
    if (cf_id >= cf_.size()) return false;
    return cur_log_index < cf_[cf_id].applied_index.GetLogIndex();
  }

  void Update(size_t cf_id, LogIndex cur_log_index, SequenceNumber cur_seqno) {
    if (cf_id >= cf_.size()) return;
    if (cf_[cf_id].flushed_index <= last_flush_index_ && cf_[cf_id].flushed_index == cf_[cf_id].applied_index) {
      auto flush_log_index = std::max(cf_[cf_id].flushed_index.GetLogIndex(), last_flush_index_.GetLogIndex());
      auto flush_sequence_number =
          std::max(cf_[cf_id].flushed_index.GetSequenceNumber(), last_flush_index_.GetSequenceNumber());
      cf_[cf_id].flushed_index.SetLogIndexSeqnoPair(flush_log_index, flush_sequence_number);
    }

    cf_[cf_id].applied_index.SetLogIndexSeqnoPair(cur_log_index, cur_seqno);
  }

  bool IsPendingFlush() const;

  size_t GetPendingFlushGap() const;

  void SetLastFlushIndex(LogIndex flushed_logindex, SequenceNumber flushed_seqno) {
    auto lastest_flush_log_index = std::max(last_flush_index_.GetLogIndex(), flushed_logindex);
    auto lastest_flush_sequence_number = std::max(last_flush_index_.GetSequenceNumber(), flushed_seqno);
    last_flush_index_.SetLogIndexSeqnoPair(lastest_flush_log_index, lastest_flush_sequence_number);
  }

  // for gtest
  LogIndexSeqnoPair &GetLastFlushIndex() { return last_flush_index_; }

  LogIndexPair &GetCFStatus(size_t cf) { return cf_[cf]; }

 private:
  std::vector<LogIndexPair> cf_;  // Dynamic size based on actual CF count
  LogIndexSeqnoPair last_flush_index_;
};

class LogIndexAndSequenceCollector {
 public:
  explicit LogIndexAndSequenceCollector(uint8_t step_length_bit = 0) { step_length_mask_ = (1 << step_length_bit) - 1; }

  // find the index of log which contain seqno or before it
  LogIndex FindAppliedLogIndex(SequenceNumber seqno) const;

  // if there's a new pair, add it to list; otherwise, do nothing
  void Update(LogIndex smallest_applied_log_index, SequenceNumber smallest_flush_seqno);

  // purge out dated log index after memtable flushed.
  void Purge(LogIndex smallest_applied_log_index);

  // Is manual flushing required?
  bool IsFlushPending() const { return GetSize() >= max_gap_; }

  // for gtest
  uint64_t GetSize() const {
    std::shared_lock<std::shared_mutex> share_lock(mutex_);
    return list_.size();
  }

  std::deque<LogIndexAndSequencePair> &GetList() {
    std::shared_lock<std::shared_mutex> share_lock(mutex_);
    return list_;
  }

 public:
  static std::atomic_int64_t max_gap_;

 private:
  uint64_t step_length_mask_ = 0;
  mutable std::shared_mutex mutex_;
  std::deque<LogIndexAndSequencePair> list_;
};

class LogIndexTablePropertiesCollector : public rocksdb::TablePropertiesCollector {
 public:
  static constexpr std::string_view kPropertyName = "LargestLogIndex/LargestSequenceNumber";

  explicit LogIndexTablePropertiesCollector(const LogIndexAndSequenceCollector &collector) : collector_(collector) {}

  rocksdb::Status AddUserKey(const rocksdb::Slice &key, const rocksdb::Slice &value, rocksdb::EntryType type,
                             SequenceNumber seq, uint64_t file_size) override {
    largest_seqno_ = std::max(largest_seqno_, seq);
    return rocksdb::Status::OK();
  }
  rocksdb::Status Finish(rocksdb::UserCollectedProperties *properties) override {
    properties->insert(Materialize());
    return rocksdb::Status::OK();
  }
  const char *Name() const override { return "LogIndexTablePropertiesCollector"; }
  rocksdb::UserCollectedProperties GetReadableProperties() const override {
    return rocksdb::UserCollectedProperties{Materialize()};
  }

  static std::optional<LogIndexAndSequencePair> ReadStatsFromTableProps(
      const std::shared_ptr<const rocksdb::TableProperties> &table_props);

  static auto GetLargestLogIndexFromTableCollection(const rocksdb::TablePropertiesCollection &collection)
      -> std::optional<LogIndexAndSequencePair>;

 private:
  std::pair<std::string, std::string> Materialize() const {
    if (-1 == cache_) {
      cache_ = collector_.FindAppliedLogIndex(largest_seqno_);
    }
    return std::make_pair(static_cast<std::string>(kPropertyName), 
                         std::to_string(cache_) + "/" + std::to_string(largest_seqno_));
  }

 private:
  const LogIndexAndSequenceCollector &collector_;
  SequenceNumber largest_seqno_ = 0;
  mutable LogIndex cache_{-1};
};

class LogIndexTablePropertiesCollectorFactory : public rocksdb::TablePropertiesCollectorFactory {
 public:
  explicit LogIndexTablePropertiesCollectorFactory(const LogIndexAndSequenceCollector &collector)
      : collector_(collector) {}
  ~LogIndexTablePropertiesCollectorFactory() override = default;

  rocksdb::TablePropertiesCollector *CreateTablePropertiesCollector(
      [[maybe_unused]] rocksdb::TablePropertiesCollectorFactory::Context context) override {
    return new LogIndexTablePropertiesCollector(collector_);
  }
  const char *Name() const override { return "LogIndexTablePropertiesCollectorFactory"; }

 private:
  const LogIndexAndSequenceCollector &collector_;
};

class LogIndexAndSequenceCollectorPurger : public rocksdb::EventListener {
 public:
  explicit LogIndexAndSequenceCollectorPurger(std::vector<rocksdb::ColumnFamilyHandle *> *column_families,
                                              LogIndexAndSequenceCollector *collector, LogIndexOfColumnFamilies *cf,
                                              std::function<void(int64_t, bool)> callback)
      : column_families_(column_families), collector_(collector), cf_(cf), callback_(callback) {}

  void OnFlushCompleted(rocksdb::DB *db, const rocksdb::FlushJobInfo &flush_job_info) override;

 private:
  std::vector<rocksdb::ColumnFamilyHandle *> *column_families_ = nullptr;
  LogIndexAndSequenceCollector *collector_ = nullptr;
  LogIndexOfColumnFamilies *cf_ = nullptr;
  std::atomic_uint64_t count_ = 0;
  std::atomic<size_t> manul_flushing_cf_ = -1;
  std::function<void(int64_t, bool)> callback_;
};

}  // namespace storage
