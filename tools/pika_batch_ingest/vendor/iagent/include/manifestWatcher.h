#pragma once
#include <deque>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_set>

namespace iagent {

class ManifestWatcher {
public:
  ManifestWatcher(const std::string &queueFilePath,
                  const std::string &offsetFilePath);

  void enqueue(const std::string &content);
  bool hasPending();
  std::string next();
  std::string popNext();
  void ack();
  void ack(size_t n);

private:
  void loadOffset();
  void loadQueueFromDisk();
  void persistEnqueue(const std::string &content);
  void persistOffset();

private:
  std::string queueFilePath_;
  std::string offsetFilePath_;
  size_t currentOffset_{0};
  std::deque<std::string> ready_;
  std::deque<std::string> staged_;
  std::unordered_set<std::string> seen_;

  std::mutex mutex_;
};

} // namespace iagent
