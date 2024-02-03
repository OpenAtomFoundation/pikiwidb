#include "src/log_index.h"

namespace storage {

void LogIndexCollector::Update(int64_t applied_log_index, rocksdb::SequenceNumber seqno) {
  /*
    If step length > 1, log index is sampled and sacrifice precision to save memory usage.
    It means that extra applied log may be applied again on start stage.
    For example, if we use step length 100, we only use 1/100 memory, then at most extra
    100 applied log may be applied again on start stage.
  */
  if (step_length_ == 1 || ++num_update_ % step_length_ == 0) {
    std::lock_guard<std::mutex> guard(mutex_);
    LogIndex log_index(applied_log_index, seqno);
    collector_.emplace_back(log_index);
  }
}

int64_t LogIndexCollector::FindAppliedLogIndex(rocksdb::SequenceNumber seqno) {
  if (seqno == 0) {
    return 0;
  }

  std::lock_guard<std::mutex> guard(mutex_);
  int64_t applied_log_index = 0;

  for (auto it = collector_.begin(); it != collector_.end(); it++) {
    if (seqno >= it->GetSequenceNumber()) {
      applied_log_index = it->GetAppliedLogIndex();
    } else {
      break;
    }
  }

  return applied_log_index;
}

void LogIndexCollector::Purge(int64_t applied_log_index) {
  std::lock_guard<std::mutex> guard(mutex_);

  while (!collector_.empty()) {
    auto &r = collector_.front();
    if (applied_log_index >= r.GetAppliedLogIndex()) {
      collector_.pop_front();
    } else {
      break;
    }
  }
}

const std::string LogIndexTablePropertiesCollector::properties_name_ = "lastest_applied_log_index/largest_seqno";

rocksdb::Status LogIndexTablePropertiesCollector::AddUserKey(const rocksdb::Slice &key, const rocksdb::Slice &value,
                                                             +rocksdb::EntryType type, rocksdb::SequenceNumber seq,
                                                             +uint64_t file_size) {
  smallest_seqno_ = std::min(smallest_seqno_, seq);
  largest_seqno_ = std::max(largest_seqno_, seq);
  return rocksdb::Status::OK();
}

rocksdb::Status LogIndexTablePropertiesCollector::Finish(rocksdb::UserCollectedProperties *const properties) {
  properties->insert(materialize());
  return rocksdb::Status::OK();
}

rocksdb::UserCollectedProperties LogIndexTablePropertiesCollector::GetReadableProperties() const {
  return rocksdb::UserCollectedProperties{materialize()};
}

void LogIndexTablePropertiesCollector::ReadStatsFromTableProps(
    const std::shared_ptr<const rocksdb::TableProperties> &table_props, int64_t &applied_log_index) {
  const auto &user_properties = table_props->user_collected_properties;
  const auto it = user_properties.find(properties_name_);
  if (it != user_properties.end()) {
    std::string s = it->second;
    rocksdb::SequenceNumber largest_seqno;
    sscanf(s.c_str(), "%" PRIi64 "/%" PRIu64 "", &applied_log_index, &largest_seqno);
  }
}

std::pair<std::string, std::string> LogIndexTablePropertiesCollector::materialize() const {
  char buf[64];
  int64_t applied_log_index = 0;
  if (tmp_.count(largest_seqno_) != 0) {
    applied_log_index = tmp_[largest_seqno_];
  } else {
    applied_log_index = collector_->Find(largest_seqno_);
    tmp_[largest_seqno_] = applied_log_index;
  }

  snprintf(buf, 64, "%" PRIi64 "/%" PRIu64 "", applied_log_index, largest_seqno_);
  std::string buf_s = buf;
  return std::make_pair(properties_name_, buf_s);
}

}  // namespace storage