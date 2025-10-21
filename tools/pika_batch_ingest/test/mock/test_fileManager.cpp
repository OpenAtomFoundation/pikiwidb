#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <ctime>
#include "fileManager.h"
#include "utils/kvEntry.h"

namespace fs = std::filesystem;

class FileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = "/tmp/filemanager_test_" + std::to_string(std::time(nullptr));
        fs::create_directories(testDir);
    }
    
    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    std::string testDir;
};

TEST_F(FileManagerTest, Constructor) {
    mock::FileManager fileManager(testDir);
    EXPECT_TRUE(true) << "FileManager constructor test";
}

TEST_F(FileManagerTest, WriteData) {
    mock::FileManager fileManager(testDir);

    DataType testData;
    testData.push_back({"key1", "value1", 100});
    testData.push_back({"key2", "value2", 200});

    auto future = fileManager.write(testData);
    Result result = future.get(); 
    EXPECT_FALSE(result.isError()) << "Data writing failed: " << result.message();

    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    EXPECT_EQ(fileCount, 1) << "Expected exactly one file to be created";
}

TEST_F(FileManagerTest, MultipleWrites) {
    mock::FileManager fileManager(testDir);

    DataType testData1{{"key1", "value1", 100}};
    DataType testData2{{"key2", "value2", 200}};

    auto f1 = fileManager.write(testData1);
    auto f2 = fileManager.write(testData2);

    EXPECT_FALSE(f1.get().isError());
    EXPECT_FALSE(f2.get().isError());

    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    EXPECT_EQ(fileCount, 2) << "Expected two files to be created";
}

TEST_F(FileManagerTest, WriteEmptyData) {
    mock::FileManager fileManager(testDir);

    DataType emptyData;
    auto future = fileManager.write(emptyData);
    Result result = future.get();
    EXPECT_FALSE(result.isError()) << "Empty data writing failed: " << result.message();
}

TEST_F(FileManagerTest, WriteLargeData) {
    mock::FileManager fileManager(testDir);

    DataType largeData;
    for (int i = 0; i < 1000; ++i) {
        largeData.push_back({"key" + std::to_string(i), "value" + std::to_string(i), static_cast<uint32_t>(i)});
    }

    auto future = fileManager.write(largeData);
    Result result = future.get();
    EXPECT_FALSE(result.isError()) << "Large data writing failed: " << result.message();

    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    EXPECT_EQ(fileCount, 1);
}

TEST_F(FileManagerTest, FileContentFormat) {
    mock::FileManager fileManager(testDir);

    DataType testData;
    testData.push_back({"key1", "value1", 100});
    testData.push_back({"key2", "value2", 200});

    auto future = fileManager.write(testData);
    Result result = future.get();
    EXPECT_FALSE(result.isError());

    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            EXPECT_NE(content.find("key1"), std::string::npos);
            EXPECT_NE(content.find("value1"), std::string::npos);
            EXPECT_NE(content.find("key2"), std::string::npos);
            EXPECT_NE(content.find("value2"), std::string::npos);
        }
    }
}
