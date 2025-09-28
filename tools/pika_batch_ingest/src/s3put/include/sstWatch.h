#ifndef S3PUT_SSTWATCH_H
#define S3PUT_SSTWATCH_H

#include "manifestBuilder.h"
#include "sstTracker.h"
#include <ThreadPool.h>
#include <atomic>
#include <functional>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace s3put {

class SstWatcher {
public:
  using Callback = std::function<void(const std::vector<std::string> &)>;

  SstWatcher(SstTracker &tracker, const std::string &sst_root, ThreadPool &pool,
             int interval_sec);

  void SetCallback(Callback cb);
  void Start();
  void Stop();
  void SimulateChange(const std::vector<std::string> &changed);
  void ScheduledScan();

private:
  SstTracker &tracker_;
  std::string sst_root_;
  int interval_sec_;
  std::atomic<bool> running_;
  ThreadPool &pool_;
  Callback on_change_callback_;
};

} // namespace s3put

#endif // S3PUT_SSTWATCH_H
