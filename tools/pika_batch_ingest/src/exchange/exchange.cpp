#include <iostream>
#include <string>
#include <unistd.h>
#include "sstProcessor.h"
#include "JsonFileManager.h"
#include "utils/klog.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "utils/ktime.h"

namespace fs = std::filesystem;

void print_usage(const char *prog)
{
    std::cout << "Usage:\n"
              << "  " << prog << " -k <kvPath> -s <sstPath>         # Single file mode\n"
              << "  " << prog << " -d <jsonDirPath>                  # Batch directory mode\n";
}

bool isFolderListed(const std::string &target)
{
    std::ifstream inFile(DEFAULTCONFIGFILEDIC);
    if (!inFile.is_open())
    {
        LOG_ERROR("Failed to open config file: " + std::string(DEFAULTCONFIGFILEDIC));
        return false;
    }

    try
    {
        nlohmann::json configJson;
        inFile >> configJson;

        if (!configJson.contains("folders") || !configJson["folders"].is_array())
        {
            LOG_ERROR("Invalid config file format. Expecting a 'folders' array.");
            return false;
        }

        for (const auto &folder : configJson["folders"])
        {

            if (folder.is_string() && folder.get<std::string>() == target)
            {
                return true;
            }
        }

        return false;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("JSON parse error in config file: " + std::string(e.what()));
        return false;
    }
}

int main(int argc, char **argv)
{
    std::string kvPath;
    std::string sstPath;
    std::string dirPath;

    int opt;
    while ((opt = getopt(argc, argv, "k:s:d:")) != -1)
    {
        switch (opt)
        {
        case 'k':
            kvPath = optarg;
            break;
        case 's':
            sstPath = optarg;
            break;
        case 'd':
            dirPath = optarg;
            break;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    bool singleMode = !kvPath.empty() && !sstPath.empty();
    bool batchMode = !dirPath.empty();

    if ((singleMode && batchMode) || (!singleMode && !batchMode))
    {
        LOG_ERROR("Error: Invalid argument combination.");
        print_usage(argv[0]);
        return 1;
    }

    exchange::JsonFileManager fileManager;
    rocksdb::Options options;
    options.create_if_missing = true;

    exchange::SstProcessor processor(options);
    TimeTracker::Start("[@EXCHANGE]");

    if (singleMode)
    {
        LOG_DEBUG("Running in single file mode with kvPath: " + kvPath + ", sstPath: " + sstPath);

        Result result = processor.processSstFile(&fileManager, kvPath, sstPath);
        if (result.getRet() == Result::Ret::kOk)
        {
            LOG_DEBUG("Success:  " + result.message());
            TimeTracker::End();
        }
        else
        {
            LOG_ERROR("Error: " + result.message());
            return 1;
        }
    }
    else if (batchMode)
    {
        LOG_INFO("Running in multi-threaded directory mode with Input directory: " + dirPath);

        auto state_path_dir = fs::path(dirPath).filename();
        if (!fs::exists(state_path_dir))
        {
            std::error_code ec;
            if (!fs::create_directories(state_path_dir, ec) && ec)
            {
                LOG_ERROR("Failed to create directory for state_path: " + state_path_dir.string() +
                          " error: " + ec.message());
                return 1;
            }
        }
        Result result = processor.mutiProcessSstFile(&fileManager, dirPath);
        if (result.getRet() == Result::Ret::kOk)
        {
            LOG_DEBUG("Success: " + result.message());
            TimeTracker::End();
        }
        else
        {
            LOG_ERROR("Error: " + result.message());
            return 1;
        }
    }

    return 0;
}
