#include "sstProcessor.h"
#include "JsonFileManager.h"
#include "utils/compare.h"
#include <filesystem>
#include <iostream>
#include "utils/klog.h"
#include <ThreadPool.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "rocksdb/table.h"
#include "rocksdb/file_checksum.h"
#include "storage/src/strings_value_format.h"

namespace exchange
{

    namespace fs = std::filesystem;

    std::vector<std::pair<std::string, std::string>> SstProcessor::collectJsonFiles(const std::string &inputDir)
    {
        std::vector<std::pair<std::string, std::string>> result;
        for (const auto &entry : fs::recursive_directory_iterator(inputDir))
        {
            LOG_DEBUG("exchange: Checking entry: " + entry.path().string());
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                std::string jsonPath = entry.path().string();
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), DEFAULTDIC);
                std::filesystem::path sstPath = DEFAULTSSTDIC / relativePath;
                sstPath.replace_extension(".sst");

                LOG_DEBUG("exchange: Found JSON file: " + jsonPath + ", corresponding SST path: " + sstPath.string());

                result.emplace_back(jsonPath, sstPath.string());
            }
        }
        return result;
    }

    Result SstProcessor::processSstFile(JsonFileManagerBase *fileManager,
                                        const std::string &inputJsonPath,
                                        const std::string &outputSstPath)
    {
        DataType data;
        try
        {
            data = fileManager->parse(inputJsonPath); 
        }
        catch (const std::exception &e)
        {
            return Result(Result::Ret::kFileReadError, "JSON parse failed: " + std::string(e.what()));
        }
        try
        {
            fs::path outputPath(outputSstPath);
            fs::path parentDir = outputPath.parent_path();

            if (!parentDir.empty() && !fs::exists(parentDir))
            {
                LOG_DEBUG("exchange: Parent directory does not exist, creating: " + parentDir.string());
                fs::create_directories(parentDir);
            }
        }
        catch (const std::exception &e)
        {
            return Result(Result::Ret::kFileWriteError, "Failed to create directory: " + std::string(e.what()));
        }

        rocksdb::Options sst_options = options_;
        rocksdb::BlockBasedTableOptions table_options;
        table_options.checksum = rocksdb::kCRC32c;
        sst_options.file_checksum_gen_factory = rocksdb::GetFileChecksumGenCrc32cFactory();
        sst_options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
        rocksdb::SstFileWriter writer(rocksdb::EnvOptions(), sst_options, cfh_);
        auto status = writer.Open(outputSstPath);
        if (!status.ok())
        {
            return Result(Result::Ret::kFileWriteError, "Failed to open SST file: " + status.ToString());
        }
        std::sort(data.begin(), data.end(), ComparePair());
        std::vector<KvEntry> deduped;
        for (size_t i = 0; i < data.size();)
        {
            deduped.push_back(data[i]);
            size_t j = i + 1;
            while (j < data.size() && data[j].key == data[i].key)
            {
                ++j;
            }
            i = j;
        }

        size_t kvCount = 0;
        size_t totalRawBytes = 0;
        size_t totalEncodeBytes = 0;
        for (const auto &entry : deduped)
        {
            storage::StringsValue strings_value(entry.value);
            //  if (entry.timestamp > 0) {
            //     strings_value.SetRelativeTimeInMillsec(entry.timestamp);
            // }
            auto encodedVal = strings_value.Encode();
            
            status = writer.Put(rocksdb::Slice(entry.key), encodedVal);
            kvCount++;
            totalRawBytes  += entry.key.size();
            totalRawBytes += entry.value.size();

            if (!status.ok())
            {
                writer.Finish().PermitUncheckedError();
                return Result(Result::Ret::kFileWriteError, "Put failed: " + status.ToString());
            }
        }

        status = writer.Finish();
        if (!status.ok())
        {
            return Result(Result::Ret::kFileWriteError, "Finish failed: " + status.ToString());
        }
        totalKeys_.fetch_add(kvCount, std::memory_order_relaxed);
        totalRawBytes_.fetch_add(totalRawBytes, std::memory_order_relaxed);
        totalEncodeBytes_.fetch_add(totalEncodeBytes, std::memory_order_relaxed);
        return Result(Result::Ret::kOk, "SST file created successfully: " + outputSstPath);
    }

    Result SstProcessor::mutiProcessSstFile(JsonFileManagerBase *fileManager, const std::string &inputDicPath)
    {
        std::string inputDicPathFull = (fs::path(DEFAULTDIC) / inputDicPath).string();
        auto filePairs = collectJsonFiles(inputDicPathFull);
        if (filePairs.empty())
        {
            return Result(Result::Ret::kFileReadError, "No JSON files found in directory: " + inputDicPath);
        }

        ThreadPool pool(std::thread::hardware_concurrency());
        std::mutex resultMutex;
        std::vector<Result> results;

        std::vector<std::future<void>> futures;

        for (const auto &pair : filePairs)
        {
            futures.emplace_back(pool.enqueue([&, pair]()
                                              {
            Result r = processSstFile(fileManager, pair.first, pair.second);
            {
                std::lock_guard<std::mutex> lock(resultMutex);
                results.push_back(r);
                if (r.isError()) {
                    LOG_ERROR("exchange: Failed to process file: " + pair.first + " => " + r.message());
                }
                else
                {
                    LOG_INFO("exchange: Successfully processed file: " + pair.first + " => " + pair.second);
                }
            } }));
        }

        for (auto &fut : futures)
        {
            fut.get();
        }

        for (const auto &r : results)
        {
            if (r.isError())
            {
                return Result(Result::Ret::kFileWriteError, "Some files failed. Check logs.");
            }
        }

        try {
            nlohmann::json summary;
            summary["total_keys"] = totalKeys_.load();
            summary["total_raw_bytes"] = totalRawBytes_.load();
            summary["total_encode_bytes"] = totalEncodeBytes_.load();
            summary["status"] = "ok";

            std::string sstCountPath = fs::path(SUMMERMETA).string();
            std::ofstream out(sstCountPath);
            if (!out.is_open()) {
                return Result(Result::Ret::kFileWriteError, "Failed to write summary meta file: " + sstCountPath);
            }
            out << summary.dump(2);
            out.close();

            LOG_INFO("exchange: Summary meta written to " + sstCountPath);
        } catch (const std::exception &e) {
            return Result(Result::Ret::kFileWriteError, "Failed to write summary meta: " + std::string(e.what()));
        }

        return Result(Result::Ret::kOk, "All files processed successfully.");
    }

} // namespace exchange