#include "sstWatch.h"
#include "fileReader.h"
#include "utils/klog.h"
#include "utils/ktime.h"
#include <thread>

namespace s3put {

SstWatcher::SstWatcher(SstTracker &tracker, const std::string &sst_root,
                       ThreadPool &pool, int interval_sec)
    : tracker_(tracker), sst_root_(sst_root), interval_sec_(interval_sec),
      running_(false), pool_(pool) {}

void SstWatcher::SetCallback(Callback cb) {
  on_change_callback_ = std::move(cb);
}

void SstWatcher::Start() {
  if (running_.exchange(true)) {
    return; 
  }
  pool_.enqueue([this] { ScheduledScan(); });
}

void SstWatcher::Stop() { running_ = false; }

void SstWatcher::SimulateChange(const std::vector<std::string> &changed) {
  if (on_change_callback_) {
    on_change_callback_(changed);
  }
}

void SstWatcher::ScheduledScan() {
  try {
    TimeTracker::Start("[@S3PUT]");
    FileReader reader(sst_root_, std::thread::hardware_concurrency());
    reader.scan();
    auto files = reader.get_files();
    for (const auto &path : files) {
      if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".sst") == 0) {
        tracker_.HasChanged(path);
      }
    }

    auto changed = tracker_.GetChangedFiles();
    if (!changed.empty()) {
      LOG_INFO("Detected " + std::to_string(changed.size()) + " SST changes");
      if (on_change_callback_) {
        on_change_callback_(changed);
      }
    } else {
      LOG_INFO("No changes detected in SST files.");
    }
    TimeTracker::End();
  } catch (const std::exception &ex) {
    LOG_ERROR(std::string("ScheduledScan error: ") + ex.what());
  }
}

} // namespace s3put
