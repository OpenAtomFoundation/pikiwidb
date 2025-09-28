#include "manifestWatcher.h"
#include "utils/klog.h"
#include <fstream>
#include <sstream>

namespace iagent {

ManifestWatcher::ManifestWatcher(const std::string &queueFilePath,
                                 const std::string &offsetFilePath)
    : queueFilePath_(queueFilePath), offsetFilePath_(offsetFilePath) {
  loadOffset();
  loadQueueFromDisk();
}

void ManifestWatcher::loadOffset() {
  std::ifstream in(offsetFilePath_);
  if (in) {
    in >> currentOffset_;
  } else {
    currentOffset_ = 0;
  }
}

void ManifestWatcher::loadQueueFromDisk() {
  std::ifstream in(queueFilePath_);
  if (!in)
    return;

  std::string line;
  size_t lineIdx = 0;

  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    seen_.insert(line);
    if (lineIdx++ >= currentOffset_) {
      ready_.push_back(line);
    }
  }
}

void ManifestWatcher::persistEnqueue(const std::string &content) {
  std::ofstream out(queueFilePath_, std::ios::app);
  out << content << "\n";
  out.flush();
}

void ManifestWatcher::persistOffset() {
  std::ofstream out(offsetFilePath_, std::ios::trunc);
  out << currentOffset_ << "\n";
  out.flush();
}

void ManifestWatcher::enqueue(const std::string &content) {
  std::lock_guard<std::mutex> lk(mutex_);
  if (content.empty())
    return;

  if (seen_.insert(content).second) {
    ready_.push_back(content);
    persistEnqueue(content);
    LOG_DEBUG("[ManifestWatcher] Enqueueing content: " + content);
  } else {
    LOG_DEBUG("[ManifestWatcher] Duplicate ignored: " + content);
  }
}

bool ManifestWatcher::hasPending() {
  std::lock_guard<std::mutex> lk(mutex_);
  return !ready_.empty();
}

std::string ManifestWatcher::next() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (ready_.empty())
    return {};
  return ready_.front();
}

std::string ManifestWatcher::popNext() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (ready_.empty())
    return {};
  std::string k = std::move(ready_.front());
  ready_.pop_front();
  staged_.push_back(k);
  return k;
}

void ManifestWatcher::ack() {
  std::lock_guard<std::mutex> lk(mutex_);
  if (staged_.empty())
    return;

  staged_.pop_front();
  ++currentOffset_;
  persistOffset();
}

void ManifestWatcher::ack(size_t n) {
  std::lock_guard<std::mutex> lk(mutex_);
  n = std::min(n, staged_.size());
  if (n == 0)
    return;

  for (size_t i = 0; i < n; ++i) {
    staged_.pop_front();
    ++currentOffset_;
  }
  persistOffset();
}

} // namespace iagent
