#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <ctime>
#include "dataGen.h"
#include "fieldGens/FieldGebBuilder.h"
#include "utils/threadScheduler.h"

namespace fs = std::filesystem;

class DataGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = "/tmp/mock_test_" + std::to_string(std::time(nullptr));
        fs::create_directories(testDir);
        ThreadScheduler::get().init(4); 
        ThreadScheduler::get().registerLogicalRequest("dataGen", 2);
        ThreadScheduler::get().registerLogicalRequest("keyGen", 1);
        ThreadScheduler::get().registerLogicalRequest("valueGen", 1);
        ThreadScheduler::get().finalize();
    }
    
    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    std::string testDir;
};

TEST_F(DataGenTest, Constructor) {
    mock::DataGen generator(testDir, 100, 1.0, 1.0, 1.0, 2);
    EXPECT_EQ(generator.getNumThreads(), 2);
}

TEST_F(DataGenTest, GenerateDataBasic) {
    mock::DataGen generator(testDir, 100, 1.0, 1.0, 1.0, 2);
    
    auto keyGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "key_", 10, 100);
    auto valueGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "value_", 20, 100);
    
    generator.setKeyGenerator(keyGen);
    generator.setValueGenerator(valueGen);
    
    auto result = generator.generateData();
    EXPECT_FALSE(result.isError()) << "Data generation failed: " << result.message();
    
    EXPECT_TRUE(fs::exists(testDir)) << "Test directory not created";
    
    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    
    EXPECT_GT(fileCount, 0) << "No files were generated";
}

TEST_F(DataGenTest, SetFileManager) {
    mock::DataGen generator(testDir, 100, 1.0, 1.0, 1.0, 1);
    
    auto keyGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "key_", 10, 100);
    generator.setKeyGenerator(keyGen);
    
    auto valueGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "value_", 20, 100);
    generator.setValueGenerator(valueGen);
    
    auto result = generator.generateData();
    EXPECT_FALSE(result.isError()) << "Data generation failed after setting generators";
}

TEST_F(DataGenTest, GenerateDataWithZeroThreads) {
    mock::DataGen generator(testDir, 100, 1.0, 0.5, 1.0, 0);
    
    auto keyGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "key_", 10, 100);
    auto valueGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "value_", 20, 100);
    
    generator.setKeyGenerator(keyGen);
    generator.setValueGenerator(valueGen);
    
    auto result = generator.generateData();
    EXPECT_FALSE(result.isError()) << "Data generation failed with zero threads: " << result.message();
}

TEST_F(DataGenTest, GenerateSmallData) {
    mock::DataGen generator(testDir, 50, 0.1, 1.0, 0.5, 1);
    
    auto keyGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "key_", 5, 50);
    auto valueGen = mock::createFieldGenerator(mock::FieldDistributionType::Normal, "value_", 10, 50);
    
    generator.setKeyGenerator(keyGen);
    generator.setValueGenerator(valueGen);
    
    auto result = generator.generateData();
    EXPECT_FALSE(result.isError()) << "Small data generation failed: " << result.message();
    
    EXPECT_FALSE(result.isError()) << "DataGen should handle small data without error";
}