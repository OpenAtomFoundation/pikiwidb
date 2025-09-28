#ifndef S3SyncManager_H
#define S3SyncManager_H

#include "s3Uploader.h"
#include "sstTracker.h"
#include "sstWatch.h"
#include <memory>
#include <string>
#include <vector>

namespace s3put {

class S3SyncManager {
public:
  S3SyncManager();
  ~S3SyncManager();
  bool Init(const std::string &s3_config_path,
            std::unique_ptr<SstTracker> tracker = nullptr,
            std::unique_ptr<S3Uploader> uploader = nullptr,
            std::unique_ptr<SstWatcher> watcher = nullptr,
            std::unique_ptr<SstTracker> builder = nullptr);
  void Run();

  SstWatcher *GetWatcher() const { return watcher_.get(); }
  SstTracker *GetTracker() const { return tracker_.get(); }
  ManifestBuilder *GetBuilder() const { return builder_.get(); }
  std::string GetSstRoot(const std::string &config_path);

private:
  std::unique_ptr<SstTracker> tracker_;
  std::unique_ptr<S3Uploader> uploader_;
  std::unique_ptr<SstWatcher> watcher_;
  std::unique_ptr<ManifestBuilder> builder_;

  std::string sst_root_;
  ThreadPool pool_{std::max(1u, std::thread::hardware_concurrency() / 3 - 1)};
};
} // namespace s3put

#endif // S3SyncManager_H