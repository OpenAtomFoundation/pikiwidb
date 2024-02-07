#include "src/log_index.h"

#include <cinttypes>
#include <cstdint>
#include <optional>

#include "src/redis.h"
#include "storage/storage.h"

namespace storage {

rocksdb::Status LogIndexOfCF::Init(Redis *db, size_t cf_num) {
  cf_.resize(cf_num);
  for (int i = 0; i < cf_num; i++) {
    rocksdb::TablePropertiesCollection collection;
    auto s = db->GetDB()->GetPropertiesOfAllTables(db->GetColumnFamilyHandle(i), &collection);
    if (!s.ok()) {
      return s;
    }
    for (const auto &[_, props] : collection) {
      assert(props->column_family_id == i);
      auto res = LogIndexTablePropertiesCollector::ReadStatsFromTableProps(props);
      if (res.has_value()) {
        cf_[i].applied_log_index = std::max(cf_[i].applied_log_index, res->GetAppliedLogIndex());
        cf_[i].flushed_log_index = std::max(cf_[i].flushed_log_index, res->GetAppliedLogIndex());
      }
    }
  }
  return Status::OK();
}

bool LogIndexOfCF::CheckIfApplyAndSet(size_t cf_id, int64_t cur_log_index) {
  cf_[cf_id].applied_log_index = std::max(cf_[cf_id].applied_log_index, cur_log_index);
  return cur_log_index == cf_[cf_id].applied_log_index;
}

void LogIndexOfCF::SetFlushedLogIndex(size_t cf_id, int64_t log_index) { cf_[cf_id].flushed_log_index = log_index; }

int64_t LogIndexOfCF::GetSmallestLogIndex(std::function<int64_t(const LogIndexPair &)> f) const {
  int64_t smallest_log_index = std::numeric_limits<int64_t>::max();
  for (const auto &it : cf_) {
    smallest_log_index = std::min(f(it), smallest_log_index);
  }
  return smallest_log_index;
}

void LogIndexAndSequenceCollector::Update(int64_t applied_log_index, rocksdb::SequenceNumber seqno) {
  /*
    If step length > 1, log index is sampled and sacrifice precision to save memory usage.
    It means that extra applied log may be applied again on start stage.
  */
  if (applied_log_index & step_length_mask_ == 0) {
    std::lock_guard<std::mutex> guard(mutex_);
    LogIndexAndSequencePair pair(applied_log_index, seqno);
    list_.emplace_back(pair);

    if (applied_log_index & skip_length_mask_ == 0) {
      PairAndIterator s(pair, std::prev(list_.end()));
      skip_list_.emplace_back(s);
    }
  }
}

int64_t LogIndexAndSequenceCollector::FindAppliedLogIndex(rocksdb::SequenceNumber seqno) const {
  if (seqno == 0) {
    return 0;
  }

  std::lock_guard<std::mutex> guard(mutex_);
  int64_t applied_log_index = 0;
  auto it = list_.begin();

  // use skip list to find the best iterator for search seqno
  for (const auto &s : skip_list_) {
    if (seqno >= s.GetSequenceNumber()) {
      it = s.GetIterator();
    } else {
      break;
    }
  }

  for (; it != list_.end(); it++) {
    if (seqno >= it->GetSequenceNumber()) {
      applied_log_index = it->GetAppliedLogIndex();
    } else {
      break;
    }
  }

  return applied_log_index;
}

rocksdb::Status LogIndexTablePropertiesCollector::AddUserKey(const rocksdb::Slice &key, const rocksdb::Slice &value,
                                                             rocksdb::EntryType type, rocksdb::SequenceNumber seq,
                                                             uint64_t file_size) {
  smallest_seqno_ = std::min(smallest_seqno_, seq);
  largest_seqno_ = std::max(largest_seqno_, seq);
  return rocksdb::Status::OK();
}

rocksdb::Status LogIndexTablePropertiesCollector::Finish(rocksdb::UserCollectedProperties *const properties) {
  properties->insert(Materialize());
  return rocksdb::Status::OK();
}

rocksdb::UserCollectedProperties LogIndexTablePropertiesCollector::GetReadableProperties() const {
  return rocksdb::UserCollectedProperties{Materialize()};
}

std::optional<LogIndexAndSequencePair> LogIndexTablePropertiesCollector::ReadStatsFromTableProps(
    const std::shared_ptr<const rocksdb::TableProperties> &table_props) {
  const auto &user_properties = table_props->user_collected_properties;
  const auto it = user_properties.find(kPropertyName_);
  if (it == user_properties.end()) {
    return std::nullopt;
  }
  LogIndexAndSequencePair p;
  std::string s = it->second;
  int64_t applied_log_index;
  rocksdb::SequenceNumber largest_seqno;
  sscanf(s.c_str(), "%" PRIi64 "/%" PRIu64 "", &applied_log_index, &largest_seqno);
  p.SetAppliedLogIndex(applied_log_index);
  p.SetSequenceNumber(largest_seqno);
  return p;
}

std::pair<std::string, std::string> LogIndexTablePropertiesCollector::Materialize() const {
  char buf[64];
  int64_t applied_log_index = 0;
  if (tmp_.contains(largest_seqno_)) {
    applied_log_index = tmp_[largest_seqno_];
  } else {
    applied_log_index = collector_->FindAppliedLogIndex(largest_seqno_);
    tmp_[largest_seqno_] = applied_log_index;
  }

  snprintf(buf, 64, "%" PRIi64 "/%" PRIu64 "", applied_log_index, largest_seqno_);
  std::string buf_s = buf;
  return std::make_pair(kPropertyName_, buf_s);
}

void LogIndexAndSequenceCollectorPurger::OnFlushCompleted(rocksdb::DB *db,
                                                          const rocksdb::FlushJobInfo &flush_job_info) {
  cf_->SetFlushedLogIndex(flush_job_info.cf_id, collector_->FindAppliedLogIndex(flush_job_info.largest_seqno));
  auto smallest_applied_log_index = cf_->GetSmallestAppliedLogIndex();
  auto smallest_flushed_log_index = cf_->GetSmallestFlushedLogIndex();
  collector_->Purge(smallest_applied_log_index, smallest_flushed_log_index);
}

}  // namespace storage