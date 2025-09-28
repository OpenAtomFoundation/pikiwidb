#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>
#include <nlohmann/json.hpp>

#include "utils/result.h"
#include "utils/klog.h"
#include "utils/kvEntry.h"

namespace mock {

    using json = nlohmann::json;

    class FileManagerBase {
    public:
        using DataType = std::vector<KvEntry>;
        virtual ~FileManagerBase() = default;
        virtual std::future<Result> write(const DataType &data) = 0;
    };

    class FileManager : public FileManagerBase {
    public:
        explicit FileManager(const std::string &dic,
                             unsigned writer_threads = std::max(1u, std::thread::hardware_concurrency() / 2))
            : stopFlag_(false), next_index_(0) {

            dic_ = DEFAULTDIC / dic; 
            if (!std::filesystem::exists(dic_)) {
                std::filesystem::create_directories(dic_);
            }

            if (writer_threads == 0) writer_threads = 1;
            writers_.reserve(writer_threads);
            for (unsigned i = 0; i < writer_threads; ++i) {
                writers_.emplace_back(&FileManager::writerLoop, this);
            }
        }

        ~FileManager() override {
            {
                std::lock_guard<std::mutex> lk(mutex_);
                stopFlag_ = true;
            }
            cv_.notify_all();
            for (auto &t : writers_) {
                if (t.joinable()) t.join();
            }
        }

        std::future<Result> write(const DataType &data) override {
            auto promise = std::make_shared<std::promise<Result>>();
            std::filesystem::path filePath;
            {
                std::lock_guard<std::mutex> lk(name_mutex_);
                filePath = dic_ / ("data_" + std::to_string(next_index_++) + ".json");
                LOG_DEBUG(dic_.string() + " is the directory for data files.");
                LOG_DEBUG("FileManager Creating file: " + filePath.string());
            }
            {
                std::lock_guard<std::mutex> lk(mutex_);
                tasks_.emplace(Task{filePath, data, promise});
            }
            cv_.notify_one();

            return promise->get_future();
        }

    private:
        struct Task {
            std::filesystem::path path;
            DataType data; 
            std::shared_ptr<std::promise<Result>> prom;
        };

        void writerLoop() {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lk(mutex_);
                    cv_.wait(lk, [&]{ return stopFlag_ || !tasks_.empty(); });
                    if (stopFlag_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                try {
                    LOG_DEBUG("Writing data to file: " + task.path.string());
                    std::ofstream file(task.path, std::ios::out | std::ios::trunc);
                    if (!file.is_open()) {
                        LOG_ERROR("Failed to open file: " + task.path.string());
                        task.prom->set_value(Result(Result::Ret::kFileOpenError, task.path.string()));
                        continue;
                    }

                    file << "[\n";
                    for (size_t i = 0; i < task.data.size(); ++i) {
                        json obj = task.data[i];
                        file << obj.dump();
                        if (i + 1 < task.data.size()) file << ",\n";
                    }
                    file << "\n]";
                    file.close();

                    task.prom->set_value(Result(Result::Ret::kOk, task.path.string()));
                } catch (const std::exception &e) {
                    task.prom->set_value(Result(Result::Ret::kFileWriteError, e.what()));
                }
            }
        }

        std::filesystem::path dic_;
        std::atomic<size_t> next_index_;
        std::mutex name_mutex_; 

        std::queue<Task> tasks_;
        std::vector<std::thread> writers_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::atomic<bool> stopFlag_;
    };

} // namespace mock

#endif // FILEMANAGER_H
