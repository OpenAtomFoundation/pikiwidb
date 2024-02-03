#pragma once

#include <list>
#include <mutex>

#include "rocksdb/table_properties.h"
#include "rocksdb/types.h"
#include "rocksdb/utilities/table_properties_collectors.h"

namespace storage {

class LogIndex {
 public:
  LogIndex(int64_t applied_log_index, rocksdb::SequenceNumber seqno)
      : applied_log_index_(applied_log_index), seqno_(seqno) {}
  inline int64_t GetAppliedLogIndex() { return applied_log_index_; }
  inline rocksdb::SequenceNumber GetSequenceNumber() { return seqno_; }

 private:
  int64_t applied_log_index_ = 0;
  rocksdb::SequenceNumber seqno_ = 0;
};

class LogIndexCollector {
 public:
  LogIndexCollector() {}
  LogIndexCollector(int64_t step_length) : step_length_(step_length) {}

  void Update(int64_t applied_log_index, rocksdb::SequenceNumber seqno);

  int64_t FindAppliedLogIndex(rocksdb::SequenceNumber seqno);

  /*
    purge out dated log index after braft do snapshot.
  */
  void Purge(int64_t applied_log_index);

 private:
  std::list<LogIndex> collector_;
  std::mutex mutex_;
  uint64_t num_update_ = 0;
  uint64_t step_length_ = 1;
};

class LogIndexTablePropertiesCollector : public rocksdb::TablePropertiesCollector {
 public:
  LogIndexTablePropertiesCollector(const LogIndexCollector *collector) : collector_(collector) {}
  rocksdb::Status Finish(rocksdb::UserCollectedProperties *properties) override;

  const char *Name() const override { return "LogIndexTablePropertiesCollector"; }

  rocksdb::UserCollectedProperties GetReadableProperties() const override;

  static void ReadStatsFromTableProps(const std::shared_ptr<const rocksdb::TableProperties> &table_props,
                                      int64_t &applied_log_index);

  std::pair<std::string, std::string> materialize() const;

 private:
  static const std::string properties_name_;
  const LogIndexCollector *collector_;
  rocksdb::SequenceNumber smallest_seqno_ = 0;
  rocksdb::SequenceNumber largest_seqno_ = 0;
  mutable std::map<rocksdb::SequenceNumber, int64_t> tmp_;
};

class LogIndexTablePropertiesCollectorFactory : public rocksdb::TablePropertiesCollectorFactory {
 public:
  LogIndexTablePropertiesCollectorFactory(const LogIndexCollector *collector) : collector_(collector) {}
  virtual ~LogIndexTablePropertiesCollectorFactory() {}

  rocksdb::TablePropertiesCollector *CreateTablePropertiesCollector(
      rocksdb::TablePropertiesCollectorFactory::Context context ALLOW_UNUSED) override {
    return new LogIndexTablePropertiesCollector(collector_);
  }

  virtual const char *Name() const override { return "SnapshotAuxSysPropCollFactory"; }

 private:
  const LogIndexCollector *collector_;
};

}  // namespace storage