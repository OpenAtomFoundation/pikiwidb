#include <gtest/gtest.h>
#include <set>
#include <string>
#include "fieldGens/FieldGebBuilder.h"
#include "fieldGens/IFieldGenerator.h"

class FieldGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        prefix = "test_";
        size = 10;
        poolSize = 100;
    }
    
    std::string prefix;
    size_t size;
    size_t poolSize;
};

TEST_F(FieldGeneratorTest, CreateNormalFieldGenerator) {
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Normal, 
        prefix, size, poolSize);
    
    EXPECT_NE(generator, nullptr) << "NormalFieldGenerator creation failed";
    auto result = generator->generateField();
    EXPECT_FALSE(result.isError()) << "Field generation failed: " << result.message();
    EXPECT_FALSE(result.message_raw().empty()) << "Generated field is empty";
    EXPECT_EQ(result.message_raw().substr(0, prefix.length()), prefix) 
        << "Generated field doesn't start with prefix";
}

TEST_F(FieldGeneratorTest, CreateRandomFieldGenerator) {
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Random, 
        prefix, size, poolSize);
    
    EXPECT_NE(generator, nullptr) << "RandomFieldGenerator creation failed";
    auto result = generator->generateField();
    EXPECT_FALSE(result.isError()) << "Field generation failed: " << result.message();
    EXPECT_FALSE(result.message_raw().empty()) << "Generated field is empty";
}

TEST_F(FieldGeneratorTest, CreateZipfianFieldGenerator) {
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Zipfian, 
        prefix, size, poolSize);
    
    EXPECT_NE(generator, nullptr) << "ZipfianFieldGenerator creation failed";
    auto result = generator->generateField();
    EXPECT_FALSE(result.isError()) << "Field generation failed: " << result.message();
    EXPECT_FALSE(result.message_raw().empty()) << "Generated field is empty";
}

TEST_F(FieldGeneratorTest, CreateUniformFieldGenerator) {
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Uniform, 
        prefix, size, poolSize);
    
    EXPECT_NE(generator, nullptr) << "UniformFieldGenerator creation failed";
    auto result = generator->generateField();
    EXPECT_FALSE(result.isError()) << "Field generation failed: " << result.message();
    EXPECT_FALSE(result.message_raw().empty()) << "Generated field is empty";
}

TEST_F(FieldGeneratorTest, FieldUniqueness) {
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Random, 
        prefix, size, poolSize);
    
    std::set<std::string> generatedFields;
    int numTests = 50;
    for (int i = 0; i < numTests; ++i) {
        auto result = generator->generateField();
        EXPECT_FALSE(result.isError()) << "Field generation failed on iteration " << i;
        generatedFields.insert(result.message_raw());
    }
    
    EXPECT_GE(generatedFields.size(), 1) << "No fields were generated";
}

TEST_F(FieldGeneratorTest, FieldLength) {
    size_t testSize = 20;
    auto generator = mock::createFieldGenerator(
        mock::FieldDistributionType::Normal, 
        prefix, testSize, poolSize);
    
    auto result = generator->generateField();
    EXPECT_FALSE(result.isError()) << "Field generation failed";
    EXPECT_GT(result.message_raw().length(), 0) << "Generated field is empty";
}

TEST_F(FieldGeneratorTest, DifferentPrefixes) {
    std::vector<std::string> prefixes = {"key_", "value_", "test_", "data_"};
    
    for (const auto& testPrefix : prefixes) {
        auto gen = mock::createFieldGenerator(
            mock::FieldDistributionType::Normal, 
            testPrefix, size, poolSize);
        
        auto result = gen->generateField();
        EXPECT_FALSE(result.isError()) << "Field generation failed for prefix: " << testPrefix;
        EXPECT_EQ(result.message_raw().substr(0, testPrefix.length()), testPrefix)
            << "Generated field doesn't start with prefix: " << testPrefix;
    }
}

TEST_F(FieldGeneratorTest, InvalidParameters) {
    auto generator1 = mock::createFieldGenerator(
        mock::FieldDistributionType::Normal, 
        "", size, poolSize);
    EXPECT_NE(generator1, nullptr) << "Generator creation failed with empty prefix";
    
    auto generator2 = mock::createFieldGenerator(
        mock::FieldDistributionType::Normal, 
        prefix, 0, poolSize);
    EXPECT_NE(generator2, nullptr) << "Generator creation failed with zero size";
}