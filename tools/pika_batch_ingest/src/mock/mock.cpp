#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdexcept>
#include "utils/klog.h"
#include "dataGen.h"
#include "utils/result.h"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <fstream>
#include "mock.h"
#include "utils/kconfig.h"
#include "fieldGens/FieldGebBuilder.h"
#include "utils/threadScheduler.h"
#include "utils/ktime.h"

using json = nlohmann::json;
static const std::string configFile = std::filesystem::path(CONFIG_DIR) / "config.json";

static json &getConfig()
{
    static json instance;
    return instance;
}

static void loadConfig(const std::string &filePath)
{
    std::ifstream file(filePath);
    LOG_DEBUG("Loading config from: " + filePath);
    if (!file)
    {
        perror("Failed to open config file");
        LOG_ERROR("Failed to open config file: " + filePath);
    }
    file >> getConfig();
}

class MockCmd
{
public:
    std::string directory;
    std::string keyPrefix;
    std::string valuePrefix;
    double maxFileSizeMB;
    double maxSizeGB;
    double targetSizeMB;
    double keySizeBytes;
    double valueSizeBytes;
    mock::FieldDistributionType keyType_;
    mock::FieldDistributionType valueType_;
    std::string keyDist;
    std::string valueDist;
    int keyPoolSize = -1;
    int valuePoolSize = -1;

    MockCmd()
    {
        try
        {
            loadConfig(configFile);
            auto &conf = getConfig();
            if (conf.contains("targetSizeMB"))
                targetSizeMB = conf["targetSizeMB"].get<double>();
            if (conf.contains("maxFileSizeMB"))
                maxFileSizeMB = conf["maxFileSizeMB"].get<double>();
            if (conf.contains("maxSizeGB"))
                maxSizeGB = conf["maxSizeGB"].get<double>();
            if (conf.contains("directory"))
                directory = conf["directory"].get<std::string>();
            if (conf.contains("key"))
            {
                auto keyConf = conf["key"];
                keyPrefix = keyConf["prefix"].get<std::string>();
                keySizeBytes = keyConf["size"].get<double>();
                keyDist = keyConf["distribution"].get<std::string>();
                keyPoolSize = keyConf["poolSize"].get<int>();
                keyType_ = mock::parseDistribution(keyDist);
            }
            if (conf.contains("value"))
            {
                auto valueConf = conf["value"];
                valuePrefix = valueConf["prefix"].get<std::string>();
                valueSizeBytes = valueConf["size"].get<double>();
                valueDist = valueConf["distribution"].get<std::string>();
                valuePoolSize = valueConf["poolSize"].get<int>();
                valueType_ = mock::parseDistribution(valueDist);
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Failed to load configuration: " + std::string(e.what()));
        }
    }

private:
    Result parseSize(const char *sizeStr)
    {
        size_t size = 0;
        std::string str(sizeStr);
        LOG_DEBUG("Parsing target size: " + str);
        char unit = str.back();
        str.pop_back();

        try
        {
            size = stod(str);
        }
        catch (const std::exception &e)
        {
            return Result(Result::kError, "Invalid size format: " + std::string(sizeStr));
        }

        LOG_DEBUG("Parsed size: " + std::to_string(size) + " with unit: " + unit);
        if (unit == 'G')
            return Result(Result::kOk, std::to_string(size * 1024));
        else if (unit == 'M')
            return Result(Result::kOk, std::to_string(size));
        else
            return Result(Result::kError, "Invalid size unit, only 'G' or 'M' are supported");
    }
};

int main(int argc, char **argv)
{
    MockCmd cmd;

    LOG_DEBUG("Target size: " + std::to_string(cmd.targetSizeMB) + "MB");
    LOG_DEBUG("Directory: " + cmd.directory);

    if (cmd.directory.empty()) {
        LOG_ERROR("Directory not specified in config file");
        return 1;
    }

    json folderHistory = mock::LoadFolderHistory();
    if (!mock::CheckFolderNameUnique(folderHistory, cmd.directory))
    {
        LOG_ERROR("Folder name conflict detected: " + cmd.directory);
        return 1;
    }
    if (!mock::SaveFolderHistory(folderHistory, cmd.directory))
    {
        LOG_ERROR("Failed to save folder history.");
        return 1; 
    }

    try
    {
        initThreadSchedulerFromConfig(MOCKTHREADCONF);

        mock::DataGen generator(cmd.directory, (cmd.keySizeBytes + cmd.valueSizeBytes), cmd.maxFileSizeMB, cmd.targetSizeMB, cmd.maxSizeGB, ThreadScheduler::get().get("dataGen"));

        auto estimatePoolSize = [&](int poolSize, double sizeBytes) -> size_t
        {
            if (poolSize > 0)
                return static_cast<size_t>(poolSize);
            double estimatedNumEntries = (cmd.targetSizeMB * 1024 * 1024) / sizeBytes;
            return static_cast<size_t>(std::sqrt(estimatedNumEntries));
        };

        size_t estimatedKeyPoolSize = estimatePoolSize(cmd.keyPoolSize, cmd.keySizeBytes);
        size_t estimatedValuePoolSize = estimatePoolSize(cmd.valuePoolSize, cmd.valueSizeBytes);

        generator.setKeyGenerator(mock::createFieldGenerator(cmd.keyType_, cmd.keyPrefix, cmd.keySizeBytes, estimatedKeyPoolSize));
        generator.setValueGenerator(mock::createFieldGenerator(cmd.valueType_, cmd.valuePrefix, cmd.valueSizeBytes, estimatedValuePoolSize));
        TimeTracker::Start("[@MOCK]");
        
        if (!generator.getKeyGenerator() || !generator.getValueGenerator()) {
            LOG_ERROR("Failed to create key or value generator");
            return 1;
        }
        
        Result res = generator.generateData();
        TimeTracker::End();
        
        if (res.isError()) {
            LOG_ERROR("Data generation failed: " + res.message());
            return 1;
        }
        
        LOG_INFO("Data generation completed successfully.");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Data generation failed: " + std::string(e.what()));
        return 1;
    }
    return 0;
}
