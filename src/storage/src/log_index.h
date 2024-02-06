#pragma once

#include <cstdint>
#include <list>
#include <mutex>
#include <optional>

#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"

namespace storage {

class Redis;

class LogIndexOfCF {
 public:
  rocksdb::Status Init(Redis *db, size_t cf_num);

  inline bool CheckIfApplyAndSet(size_t cf_id, int64_t cur_log_index) {
    applied_log_index_[cf_id] = std::max(cur_log_index, applied_log_index_[cf_id]);
    return cur_log_index == applied_log_index_[cf_id];
  }

 private:
  std::vector<int64_t> applied_log_index_;
};

class LogIndexAndSequencePair {
 public:
  LogIndexAndSequencePair(int64_t applied_log_index, rocksdb::SequenceNumber seqno)
      : applied_log_index_(applied_log_index), seqno_(seqno) {}

  inline int64_t GetAppliedLogIndex() const { return applied_log_index_; }
  inline rocksdb::SequenceNumber GetSequenceNumber() const { return seqno_; }

 private:
  int64_t applied_log_index_ = 0;
  rocksdb::SequenceNumber seqno_ = 0;
};

class LogIndexAndSequenceCollector {
 public:
  explicit LogIndexAndSequenceCollector(int64_t step_length = 1) : step_length_(step_length) {}

  // purge out dated log index after braft do snapshot.
  void Purge(int64_t applied_log_index);
  void Update(int64_t applied_log_index, rocksdb::SequenceNumber seqno);
  int64_t FindAppliedLogIndex(rocksdb::SequenceNumber seqno) const;

 private:
  std::list<LogIndexAndSequencePair> list_;
  mutable std::mutex mutex_;
  uint64_t num_update_ = 0;
  uint64_t step_length_ = 1;
};

class LogIndexTablePropertiesCollector : public rocksdb::TablePropertiesCollector {
 public:
  explicit LogIndexTablePropertiesCollector(const LogIndexAndSequenceCollector *collector) : collector_(collector) {}

  rocksdb::Status AddUserKey(const rocksdb::Slice &key, const rocksdb::Slice &value, rocksdb::EntryType type,
                             rocksdb::SequenceNumber seq, uint64_t file_size) override;
  rocksdb::Status Finish(rocksdb::UserCollectedProperties *properties) override;
  const char *Name() const override { return "LogIndexTablePropertiesCollector"; }
  rocksdb::UserCollectedProperties GetReadableProperties() const override;

  static std::optional<int64_t> ReadLogIndexFromTableProperties(
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

}  // namespace storage