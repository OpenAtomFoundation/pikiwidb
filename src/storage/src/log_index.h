#pragma once

#include <cstdint>
#include <list>
#include <mutex>
#include <optional>

#include "rocksdb/db.h"
#include "rocksdb/listener.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"

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

class LogIndexAndSequenceOfCF {
 public:
  rocksdb::Status Init(Redis *db, size_t cf_num);

  bool CheckIfApplyAndSet(size_t cf_id, int64_t cur_log_index);
  void SetFlushedSeqno(size_t cf_id, rocksdb::SequenceNumber seqno);
  int64_t GetSmallestAppliedLogIndex();
  rocksdb::SequenceNumber GetSmallestFlushedSeqno();

 private:
  // log index: newest record in memtable.
  // seqno: newest roceord in sst file.
  std::vector<LogIndexAndSequencePair> cf_;
};

class LogIndexAndSequenceCollector {
 public:
  explicit LogIndexAndSequenceCollector(uint8_t step_length_bit = 0) {
    if (step_length_bit > 0) {
      step_length_mask_ = 1 << (step_length_bit - 1);
    }
  }

  // purge out dated log index after memtable flushed.
  void Purge(int64_t applied_log_index, rocksdb::SequenceNumber seqno);
  void Update(int64_t smallest_applied_log_index, rocksdb::SequenceNumber smallest_flush_seqno);
  int64_t FindAppliedLogIndex(rocksdb::SequenceNumber seqno) const;

 private:
  std::list<LogIndexAndSequencePair> list_;
  mutable std::mutex mutex_;
  int64_t step_length_mask_ = 0;
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
  explicit LogIndexAndSequenceCollectorPurger(LogIndexAndSequenceCollector *collector, LogIndexAndSequenceOfCF *cf)
      : collector_(collector), cf_(cf) {}
  void OnFlushCompleted(rocksdb::DB *db, const rocksdb::FlushJobInfo &flush_job_info) override;

 private:
  LogIndexAndSequenceCollector *collector_;
  LogIndexAndSequenceOfCF *cf_;
};

}  // namespace storage