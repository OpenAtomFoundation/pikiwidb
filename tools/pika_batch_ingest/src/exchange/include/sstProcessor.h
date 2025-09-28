#ifndef SST_PROCESSOR_H
#define SST_PROCESSOR_H

#include <string>
#include <memory>
#include <atomic>
#include "rocksdb/sst_file_writer.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/db_ttl.h"
#include "utils/result.h"
#include "JsonFileManager.h"

namespace exchange
{

    class SstProcessor
    {
    public:
        SstProcessor(const rocksdb::Options &opts, rocksdb::ColumnFamilyHandle *cfh = nullptr)
            : options_(opts), cfh_(cfh) {}
        Result processSstFile(JsonFileManagerBase *fileManager,
                              const std::string &inputJsonPath,
                              const std::string &outputSstPath);
        Result mutiProcessSstFile(JsonFileManagerBase *fileManager, const std::string &inputDicPath);

        std::vector<std::pair<std::string, std::string>> collectJsonFiles(const std::string &inputDir);

    private:
        rocksdb::Options options_;
        rocksdb::ColumnFamilyHandle *cfh_;
        std::atomic<size_t> totalKeys_{0};
        std::atomic<size_t> totalRawBytes_{0};
        std::atomic<size_t> totalEncodeBytes_{0};
    };

} // namespace exchange

#endif // SST_PROCESSOR_H