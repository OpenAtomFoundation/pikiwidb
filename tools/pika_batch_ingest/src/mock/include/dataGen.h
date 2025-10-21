#ifndef DATAGEN_H
#define DATAGEN_H

#include "fileManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include "utils/result.h"
#include "fieldGens/IFieldGenerator.h"

namespace mock
{
    class DataGen
    {
    public:
        DataGen(const std::string &dicPath, size_t approxEntrySize, double maxFileSizeMB, double targetSizeMB, double maxSizeMB, size_t numThreads);

        Result generateData();

        void setKeyGenerator(std::shared_ptr<IFieldGenerator> gen)
        {
            keyGen_ = std::move(gen);
        }
        
        std::shared_ptr<IFieldGenerator> getKeyGenerator() const
        {
            return keyGen_;
        }

        void setValueGenerator(std::shared_ptr<IFieldGenerator> gen)
        {
            valueGen_ = std::move(gen);
        }
        
        std::shared_ptr<IFieldGenerator> getValueGenerator() const
        {
            return valueGen_;
        }

        size_t getNumThreads() const { return numThreads_; }

        void setFileManager(const std::shared_ptr<FileManager> &fileManager)
        {
            fileManager_ = std::move(fileManager);
        }

    private:
        std::future<Result> generateFile(size_t fileSizeMB);

        Result generateKey();
        Result generateValue();

        std::shared_ptr<FileManager> fileManager_;
        std::shared_ptr<IFieldGenerator> keyGen_;
        std::shared_ptr<IFieldGenerator> valueGen_;
        size_t approxEntrySize_;
        double maxFileSizeMB_ = 256;
        double targetSizeMB_ = 0;
        double maxSizeMB_ = 0;
        size_t numThreads_ = 1;
        friend class DataGenTest;
    };

} // namespace mock

#endif // DATAGEN_H
