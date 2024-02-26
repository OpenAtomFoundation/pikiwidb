#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <utility>

#include "rocksdb/db.h"
#include "rocksdb/listener.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"
#include "storage/storage_define.h"

namespace storage {

class Redis;
class LogIndexAndSequencePair {
 public:
  LogIndexAndSequencePair() {}
  LogIndexAndSequencePair(int64_t applied_log_index, rocksdb::SequenceNumber seqno)
      : applied_log_index_(applied_log_index), seqno_(seqno) {}
  inline void SetAppliedLogIndex(int64_t applied_log_index) { applied_log_index_ = applied_log_index; }
  inline void SetSequenceNumber(rocksdb::SequenceNumber seqno) { seqno_ = seqno; }
  inline int64_t GetAppliedLogIndex() const { return applied_log_index_; }
  inline rocksdb::SequenceNumber GetSequenceNumber() const { return seqno_; }

 private:
  int64_t applied_log_index_ = 0;
  rocksdb::SequenceNumber seqno_ = 0;
};

class LogIndexOfCF {
  struct LogIndexPair {
    std::atomic<int64_t> applied_log_index = 0;
    std::atomic<int64_t> flushed_log_index = 0;
  };

 public:
  rocksdb::Status Init(Redis *db, size_t cf_num);

  bool CheckIfApplyAndSet(size_t cf_id, int64_t cur_log_index);
  void SetFlushedLogIndex(size_t cf_id, int64_t log_index);
  int64_t GetSmallestAppliedLogIndex() const {
    return GetSmallestLogIndex([](const LogIndexPair &p) { return p.applied_log_index.load(); });
  }
  int64_t GetSmallestFlushedLogIndex() const {
    return GetSmallestLogIndex([](const LogIndexPair &p) { return p.flushed_log_index.load(); });
  }

 private:
  int64_t GetSmallestLogIndex(std::function<int64_t(const LogIndexPair &)> f) const;
  // first: newest record in memtable.
  // second: newest roceord in sst file.
  std::array<LogIndexPair, kColumnFamilyNum> cf_;
};

class LogIndexAndSequenceCollector {
 private:
  mutable std::mutex mutex_;
  std::list<LogIndexAndSequencePair> list_;
  class PairAndIterator {
   public:
    PairAndIterator() {}
    PairAndIterator(LogIndexAndSequencePair pair, decltype(list_)::iterator iter) : pair_(pair), iter_(iter) {}
    inline int64_t GetAppliedLogIndex() const { return pair_.GetAppliedLogIndex(); }
    inline rocksdb::SequenceNumber GetSequenceNumber() const { return pair_.GetSequenceNumber(); }
    inline decltype(list_)::iterator GetIterator() const { return iter_; }

   private:
    LogIndexAndSequencePair pair_;
    decltype(list_)::iterator iter_;
  };
  std::list<PairAndIterator> skip_list_;
  int64_t step_length_mask_ = 0;
  int64_t skip_length_mask_ = 0;

 public:
  explicit LogIndexAndSequenceCollector(uint8_t step_length_bit = 0, uint8_t extra_skip_length_bit = 8) {
    step_length_mask_ = (1 << step_length_bit) - 1;
    skip_length_mask_ = (1 << step_length_bit + extra_skip_length_bit) - 1;
  }

  void Update(int64_t smallest_applied_log_index, rocksdb::SequenceNumber smallest_flush_seqno);
  int64_t FindAppliedLogIndex(rocksdb::SequenceNumber seqno) const;

  template <typename T>
  void Purge(std::list<T> list, int64_t smallest_applied_log_index, int64_t smallest_flushed_log_index) {
    // purge condition:
    // We found first pair is greater than both smallest_flushed_log_index and smallest_applied_log_index,
    // then we keep previous one, and purge everyone before previous one.
    while (list.size() >= 2) {
      auto cur = list.begin();
      auto next = std::next(cur);
      if (smallest_flushed_log_index > cur->GetAppliedLogIndex() &&
          smallest_applied_log_index > next->GetAppliedLogIndex()) {
        list.pop_front();
      } else {
        break;
      }
    }
  }

  // purge out dated log index after memtable flushed.
  void Purge(int64_t smallest_applied_log_index, int64_t smallest_flushed_log_index) {
    std::lock_guard<std::mutex> guard(mutex_);
    Purge(list_, smallest_applied_log_index, smallest_flushed_log_index);
    Purge(skip_list_, smallest_applied_log_index, smallest_flushed_log_index);
  }
};

class LogIndexTablePropertiesCollector : public rocksdb::TablePropertiesCollector {
 public:
  explicit LogIndexTablePropertiesCollector(const LogIndexAndSequenceCollector *collector) : collector_(collector) {}

  rocksdb::Status AddUserKey(const rocksdb::Slice &key, const rocksdb::Slice &value, rocksdb::EntryType type,
                             rocksdb::SequenceNumber seq, uint64_t file_size) override;
  rocksdb::Status Finish(rocksdb::UserCollectedProperties *properties) override;
  const char *Name() const override { return "LogIndexTablePropertiesCollector"; }
  rocksdb::UserCollectedProperties GetReadableProperties() const override;

  static std::optional<LogIndexAndSequencePair> ReadStatsFromTableProps(
      const std::shared_ptr<const rocksdb::TableProperties> &table_props);
  static const inline std::string kPropertyName_{"latest-applied-log-index/largest-sequence-number"};

 private:
  std::pair<std::string, std::string> Materialize() const;

 private:
  const LogIndexAndSequenceCollector *collector_;
  rocksdb::SequenceNumber smallest_seqno_ = 0;
  rocksdb::SequenceNumber largest_seqno_ = 0;
  mutable std::map<rocksdb::SequenceNumber, int64_t> tmp_;
};

class LogIndexTablePropertiesCollectorFactory : public rocksdb::TablePropertiesCollectorFactory {
 public:
  explicit LogIndexTablePropertiesCollectorFactory(const LogIndexAndSequenceCollector *collector)
      : collector_(collector) {}
  ~LogIndexTablePropertiesCollectorFactory() override = default;

  rocksdb::TablePropertiesCollector *CreateTablePropertiesCollector(
      [[maybe_unused]] rocksdb::TablePropertiesCollectorFactory::Context context) override {
    return new LogIndexTablePropertiesCollector(collector_);
  }

  const char *Name() const override { return "LogIndexTablePropertiesCollectorFactory"; }

 private:
  const LogIndexAndSequenceCollector *collector_;
};

class LogIndexAndSequenceCollectorPurger : public rocksdb::EventListener {
 public:
  explicit LogIndexAndSequenceCollectorPurger(LogIndexAndSequenceCollector *collector, LogIndexOfCF *cf)
      : collector_(collector), cf_(cf) {}
  void OnFlushCompleted(rocksdb::DB *db, const rocksdb::FlushJobInfo &flush_job_info) override;

 private:
  LogIndexAndSequenceCollector *collector_;
  LogIndexOfCF *cf_;
};

}  // namespace storage