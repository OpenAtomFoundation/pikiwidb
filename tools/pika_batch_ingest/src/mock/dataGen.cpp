#include "dataGen.h"
#include "fileManager.h"
#include "utils/compare.h"
#include "utils/klog.h"
#include "utils/result.h"
#include <ThreadPool.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mock {
namespace fs = std::filesystem;

DataGen::DataGen(const std::string &dicPath, size_t approxEntrySize,
                 double maxFileSizeMB, double targetSizeMB, double maxSizeMB,
                 size_t numThreads)
    : fileManager_(std::make_shared<FileManager>(dicPath)),
      maxFileSizeMB_(maxFileSizeMB), targetSizeMB_(targetSizeMB),
      maxSizeMB_(maxSizeMB), approxEntrySize_(approxEntrySize),
      numThreads_(numThreads) {}

Result DataGen::generateData() {
  double totalDataSize = targetSizeMB_;
  double totalFiles =
      (maxFileSizeMB_ > 0) ? totalDataSize / maxFileSizeMB_ : 0.0;
  double remainder =
      (maxFileSizeMB_ > 0) ? std::fmod(totalDataSize, maxFileSizeMB_) : 0.0;
  double perFileDataSize = maxFileSizeMB_;

  LOG_DEBUG("Total data size: " + std::to_string(totalDataSize) +
            " MB, Total files to generate: " + std::to_string(totalFiles) +
            ", Remainder: " + std::to_string(remainder) +
            " MB, Per file data size: " + std::to_string(perFileDataSize) +
            " MB");

  size_t workerThreads = (numThreads_ > 1) ? (numThreads_ - 1) : 1;
  ThreadPool pool(workerThreads);
  LOG_DEBUG("numThreads: " + std::to_string(numThreads_));
  std::vector<std::future<Result>> futures;

  for (size_t i = 1; i < totalFiles; ++i) {
      futures.emplace_back(pool.enqueue([this, perFileDataSize] {
          return this->generateFile(perFileDataSize).get();
      }));
  }

  futures.emplace_back(pool.enqueue([this, perFileDataSize, remainder] {
      return this->generateFile(perFileDataSize + remainder).get();
  }));
  bool hasError = false;
  std::string errMsg;
  for (auto &f : futures) {
      Result r = f.get();
      if (r.isError()) {
          LOG_WARN("Data file write failed: " + r.message());
          hasError = true;
          errMsg = r.message();
      }
  }

  if (hasError) {
      return Result(Result::Ret::kFileWriteError, errMsg);
  }
  return Result(Result::Ret::kOk, "Data generation completed successfully.");

}


std::future<Result> DataGen::generateFile(size_t fileSizeMB) {
    DataType data;

    size_t numEntries = fileSizeMB * 1024 * 1024 / approxEntrySize_;

    LOG_DEBUG("Thread " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
              " started generating file.");
    const size_t maxRetries = 99999;
    size_t retries = 0;

    for (size_t i = 0; i < numEntries && retries < maxRetries;) {
        KvEntry entry;
        try {
            auto keyResult = generateKey();
            if (keyResult.isError()) {
                LOG_WARN("Key generation error: " + keyResult.message());
                retries++;
                continue;
            }
            entry.key = keyResult.message_raw();

            auto valueResult = generateValue();
            if (valueResult.isError()) {
                LOG_WARN("Value generation error: " + valueResult.message());
                retries++;
                continue;
            }
            entry.value = valueResult.message_raw();

            data.emplace_back(entry);
            i++;
            retries = 0;
        } catch (const std::exception &e) {
            retries++;
            LOG_WARN("Failed to generate kv: " + std::string(e.what()));
        }
    }

    if (retries >= maxRetries) {
        std::promise<Result> p;
        p.set_value(Result(Result::Ret::kError,
                           "Exceeded maximum retries for KV generation"));
        return p.get_future();
    }

    std::sort(data.begin(), data.end(), ComparePair());

    if (data.empty()) {
        std::promise<Result> p;
        p.set_value(Result(Result::Ret::kOk, "No data generated for file."));
        return p.get_future();
    }
    return fileManager_->write(data);
}



Result DataGen::generateKey() {
  if (!keyGen_)
    return Result(Result::Ret::kError, "Key generator not set");
  return keyGen_->generateField();
}

Result DataGen::generateValue() {
  if (!valueGen_)
    return Result(Result::Ret::kError, "Value generator not set");
  return valueGen_->generateField();
}

} // namespace mock