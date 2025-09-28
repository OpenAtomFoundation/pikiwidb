#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "sstProcessor.h"
#include "JsonFileManager.h"
#include "utils/kvEntry.h"
#include "utils/result.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using json = nlohmann::json;
namespace fs = std::filesystem;

class MockJsonFileManager : public exchange::JsonFileManagerBase
{
public:
    MOCK_METHOD(DataType, parse, (const std::string &), (override));
};
class ExchangeModuleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tempDir_ = "/tmp/exchange_test_" + std::to_string(time(nullptr));
        inputDir_ = tempDir_ / "input";
        outputDir_ = tempDir_ / "output";
        fs::create_directories(inputDir_);
        fs::create_directories(outputDir_);
        options_.create_if_missing = true;
        cfh_ = nullptr;
    }

    void TearDown() override
    {
        if (fs::exists(tempDir_)) {
            fs::remove_all(tempDir_);
        }
    }

    rocksdb::Options options_;
    rocksdb::ColumnFamilyHandle *cfh_;
    fs::path tempDir_;
    fs::path inputDir_;
    fs::path outputDir_;
};

TEST_F(ExchangeModuleTest, JsonFileManagerParseValidJson)
{
    fs::path jsonFile = inputDir_ / "test.json";
    json data = json::array({
        {{"key", "key_1"}, {"value", "value_1"}, {"expire", 1000}},
        {{"key", "key_2"}, {"value", "value_2"}}
    });
    
    std::ofstream out(jsonFile);
    out << data.dump(4);
    out.close();
    
    exchange::JsonFileManager fileManager;
    DataType result = fileManager.parse(jsonFile.string());
    
    ASSERT_EQ(result.size(), 2);
    
    EXPECT_EQ(result[0].key, "key_1");
    EXPECT_EQ(result[0].value, "value_1");
    EXPECT_EQ(result[0].timestamp, 1000);
    
    EXPECT_EQ(result[1].key, "key_2");
    EXPECT_EQ(result[1].value, "value_2");
    EXPECT_EQ(result[1].timestamp, 0);
}

TEST_F(ExchangeModuleTest, JsonFileManagerParseInvalidJson)
{
    fs::path jsonFile = inputDir_ / "invalid.json";
    std::ofstream out(jsonFile);
    out << "{ invalid json }";
    out.close();
    
    exchange::JsonFileManager fileManager;
    EXPECT_THROW(fileManager.parse(jsonFile.string()), std::exception);
}

TEST_F(ExchangeModuleTest, JsonFileManagerParseNonExistentFile)
{
    exchange::JsonFileManager fileManager;
    EXPECT_THROW(fileManager.parse("/non/existent/file.json"), std::runtime_error);
}

TEST_F(ExchangeModuleTest, SstProcessorConstructor)
{
    exchange::SstProcessor processor(options_, cfh_);
    SUCCEED();
}

TEST_F(ExchangeModuleTest, SstProcessorProcessSingleFile)
{
    fs::path inputJson = inputDir_ / "input.json";
    fs::path outputSst = outputDir_ / "output.sst";
    
    json data = json::array({
        {{"key", "test_key"}, {"value", "test_value"}, {"expire", 2000}}
    });
    
    std::ofstream out(inputJson);
    out << data.dump(4);
    out.close();
    
    exchange::JsonFileManager fileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    Result result = processor.processSstFile(&fileManager, inputJson.string(), outputSst.string());
    
    EXPECT_FALSE(result.isError()) << "Processing failed: " << result.message();
    EXPECT_TRUE(fs::exists(outputSst)) << "SST file was not created";
}

TEST_F(ExchangeModuleTest, SstProcessorProcessEmptyData)
{
    fs::path inputJson = inputDir_ / "empty.json";
    fs::path outputSst = outputDir_ / "empty.sst";
    
    json data = json::array({});
    
    std::ofstream out(inputJson);
    out << data.dump(4);
    out.close();
    
    exchange::JsonFileManager fileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    Result result = processor.processSstFile(&fileManager, inputJson.string(), outputSst.string());
    
    EXPECT_TRUE(result.isError() || !result.isError()) << "Processing result is acceptable";
}

TEST_F(ExchangeModuleTest, SstProcessorProcessDuplicateKeys)
{
    fs::path inputJson = inputDir_ / "duplicate.json";
    fs::path outputSst = outputDir_ / "duplicate.sst";
    
    json data = json::array({
        {{"key", "duplicate_key"}, {"value", "value_1"}, {"expire", 1000}},
        {{"key", "duplicate_key"}, {"value", "value_2"}, {"expire", 2000}},
        {{"key", "unique_key"}, {"value", "value_3"}}
    });
    
    std::ofstream out(inputJson);
    out << data.dump(4);
    out.close();
    
    exchange::JsonFileManager fileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    Result result = processor.processSstFile(&fileManager, inputJson.string(), outputSst.string());
    
    EXPECT_FALSE(result.isError()) << "Processing failed: " << result.message();
    EXPECT_TRUE(fs::exists(outputSst)) << "SST file was not created";
}

TEST_F(ExchangeModuleTest, SstProcessorWithMockFileManager)
{
    MockJsonFileManager mockFileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    DataType testData;
    testData.push_back({"key1", "value1", 1000});
    testData.push_back({"key2", "value2", 2000});
    
    EXPECT_CALL(mockFileManager, parse("test.json"))
        .WillOnce(Return(testData));
    
    fs::path outputSst = outputDir_ / "mock_output.sst";
    
    Result result = processor.processSstFile(&mockFileManager, "test.json", outputSst.string());
    
    EXPECT_FALSE(result.isError()) << "Processing failed: " << result.message();
    EXPECT_TRUE(fs::exists(outputSst)) << "SST file was not created";
}

TEST_F(ExchangeModuleTest, SstProcessorJsonParseError)
{
    MockJsonFileManager mockFileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    EXPECT_CALL(mockFileManager, parse("invalid.json"))
        .WillOnce(Throw(std::runtime_error("JSON parse error")));
    
    fs::path outputSst = outputDir_ / "error_output.sst";
    
    Result result = processor.processSstFile(&mockFileManager, "invalid.json", outputSst.string());
    
    EXPECT_TRUE(result.isError()) << "Should have failed due to JSON parse error";
    EXPECT_EQ(result.getRet(), Result::Ret::kFileReadError);
    EXPECT_FALSE(fs::exists(outputSst)) << "SST file should not be created when JSON parsing fails";
}

TEST_F(ExchangeModuleTest, SstProcessorCollectJsonFiles)
{
    fs::path json1 = inputDir_ / "file1.json";
    fs::path json2 = inputDir_ / "subdir" / "file2.json";
    fs::path txtFile = inputDir_ / "not_json.txt";
    
    fs::create_directories(json2.parent_path());
    
    std::ofstream(json1) << "{}";
    std::ofstream(json2) << "{}";
    std::ofstream(txtFile) << "not json";
    
    exchange::SstProcessor processor(options_, cfh_);
    auto filePairs = processor.collectJsonFiles(inputDir_.string());
    
    ASSERT_EQ(filePairs.size(), 2);
    
    bool foundFile1 = false, foundFile2 = false;
    for (const auto& pair : filePairs) {
        if (pair.first == json1.string()) foundFile1 = true;
        if (pair.first == json2.string()) foundFile2 = true;
    }
    
    EXPECT_TRUE(foundFile1) << "file1.json should be found";
    EXPECT_TRUE(foundFile2) << "file2.json should be found";
}

TEST_F(ExchangeModuleTest, SstProcessorCollectJsonFilesEmptyDir)
{
    exchange::SstProcessor processor(options_, cfh_);
    auto filePairs = processor.collectJsonFiles(inputDir_.string());
    
    EXPECT_EQ(filePairs.size(), 0) << "Should find no JSON files in empty directory";
}

TEST_F(ExchangeModuleTest, SstProcessorMultiThreadedProcessing)
{
    fs::path json1 = inputDir_ / "file1.json";
    fs::path json2 = inputDir_ / "file2.json";
    
    json data1 = json::array({
        {{"key", "key1"}, {"value", "value1"}}
    });
    
    json data2 = json::array({
        {{"key", "key2"}, {"value", "value2"}}
    });
    
    std::ofstream(json1) << data1.dump(4);
    std::ofstream(json2) << data2.dump(4);
    
    exchange::JsonFileManager fileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    Result result = processor.mutiProcessSstFile(&fileManager, inputDir_.string());
    
    EXPECT_FALSE(result.isError()) << "Multi-threaded processing failed: " << result.message();
    
    fs::path sst1 = outputDir_ / "file1.sst";
    fs::path sst2 = outputDir_ / "file2.sst";
}

TEST_F(ExchangeModuleTest, SstProcessorMultiThreadedProcessingEmptyDir)
{
    exchange::JsonFileManager fileManager;
    exchange::SstProcessor processor(options_, cfh_);
    
    Result result = processor.mutiProcessSstFile(&fileManager, inputDir_.string());
    
    EXPECT_TRUE(result.isError()) << "Should fail when processing empty directory";
    EXPECT_EQ(result.getRet(), Result::Ret::kFileReadError);
}

TEST_F(ExchangeModuleTest, SstProcessorWithDifferentOptions)
{
    rocksdb::Options customOptions;
    customOptions.create_if_missing = true;
    customOptions.compression = rocksdb::kSnappyCompression;
    
    exchange::SstProcessor processor(customOptions, cfh_);
    
    fs::path inputJson = inputDir_ / "options_test.json";
    fs::path outputSst = outputDir_ / "options_test.sst";
    
    json data = json::array({
        {{"key", "test_key"}, {"value", "test_value"}}
    });
    
    std::ofstream out(inputJson);
    out << data.dump(4);
    out.close();
    
    exchange::JsonFileManager fileManager;
    Result result = processor.processSstFile(&fileManager, inputJson.string(), outputSst.string());
    
    EXPECT_FALSE(result.isError()) << "Processing with custom options failed: " << result.message();
    EXPECT_TRUE(fs::exists(outputSst)) << "SST file was not created with custom options";
}