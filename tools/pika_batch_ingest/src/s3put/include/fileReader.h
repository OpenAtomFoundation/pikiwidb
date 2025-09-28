#include "utils/klog.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace s3put {
namespace fs = std::filesystem;

class FileReader {
public:
  FileReader(const std::string &root_dir, size_t num_threads)
      : root_dir_(root_dir), num_threads_(num_threads ? num_threads : 1),
        done_(false) {}

  void scan() {
    LOG_DEBUG("Starting concurrent scan in: " + root_dir_);

    {
      std::lock_guard<std::mutex> lk(files_mutex_);
      files_.clear();
    }
    total_files_.store(0);

    std::vector<std::thread> workers;
    workers.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
      workers.emplace_back([this] { this->consumer_loop(); });
    }

    producer_walk();

    {
      std::lock_guard<std::mutex> lk(q_mutex_);
      done_ = true;
    }
    q_cv_.notify_all();

    for (auto &t : workers)
      t.join();
  }

  std::vector<std::string> get_files() const {
    std::lock_guard<std::mutex> lk(files_mutex_);
    return files_;
  }

private:
  void enqueue(std::string path) {
    {
      std::lock_guard<std::mutex> lk(q_mutex_);
      queue_.push(std::move(path));
    }
    q_cv_.notify_one();
  }

  bool dequeue(std::string &out) {
    std::unique_lock<std::mutex> lk(q_mutex_);
    q_cv_.wait(lk, [this] { return !queue_.empty() || done_; });
    if (queue_.empty())
      return false; 
    out = std::move(queue_.front());
    queue_.pop();
    return true;
  }

  void producer_walk() {
    size_t discovered = 0;
    try {
      for (auto it = fs::recursive_directory_iterator(
               root_dir_, fs::directory_options::skip_permission_denied);
           it != fs::recursive_directory_iterator(); ++it) {
        std::error_code ec;
        if (!it->is_regular_file(ec))
          continue;

        auto ext = it->path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".sst") {
          enqueue(it->path().string());
          ++discovered;
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "[WARN] producer_walk exception: " << e.what() << std::endl;
    }
    std::cout << "[DEBUG] Total .sst files discovered: " << discovered
              << std::endl;
  }

  void consumer_loop() {
    std::vector<std::string> local_files;
    local_files.reserve(256);

    std::string path;
    while (true) {
      if (!dequeue(path))
        break; 

      local_files.push_back(std::move(path));
      total_files_.fetch_add(1, std::memory_order_relaxed);
      if (local_files.size() >= 256) {
        flush_local(local_files);
      }
    }
    flush_local(local_files);
  }

  void flush_local(std::vector<std::string> &local) {
    if (local.empty())
      return;
    std::lock_guard<std::mutex> lk(files_mutex_);
    files_.insert(files_.end(), std::make_move_iterator(local.begin()),
                  std::make_move_iterator(local.end()));
    local.clear();
  }

private:
  std::string root_dir_;
  size_t num_threads_;
  mutable std::mutex files_mutex_;
  std::vector<std::string> files_;
  std::atomic<int> total_files_{0};
  std::queue<std::string> queue_;
  std::mutex q_mutex_;
  std::condition_variable q_cv_;
  bool done_;
};

} // namespace s3put
