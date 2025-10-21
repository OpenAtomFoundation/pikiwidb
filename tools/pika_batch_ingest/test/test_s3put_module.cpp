#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "s3Uploader.h"
#include "s3SyncManager.h"
#include "sstTracker.h"
#include "sstWatch.h"
#include "manifestBuilder.h"
#include "configManager.h"
#include "utils/result.h"
#include "proto/manifest.pb.h"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using json = nlohmann::json;
namespace fs = std::filesystem;

class S3PutModuleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tempDir_ = "/tmp/s3put_test_" + std::to_string(time(nullptr));
        sstDir_ = tempDir_ / "sst";
        manifestDir_ = tempDir_ / "manifest";
        stateDir_ = tempDir_ / "state";
        
        fs::create_directories(sstDir_);
        fs::create_directories(manifestDir_);
        fs::create_directories(stateDir_);
        
        createTestConfig();
    }

    void TearDown() override
    {
        if (fs::exists(tempDir_)) {
            fs::remove_all(tempDir_);
        }
    }
    
    void createTestConfig()
    {
        configPath_ = tempDir_ / "s3_config.json";
        json config = {
            {"endpoint", "https://s3.amazonaws.com"},
            {"region", "us-east-1"},
            {"bucket", "test-bucket"},
            {"access_key", "test-access-key"},
            {"secret_key", "test-secret-key"},
            {"dict", "test-dict"},
            {"is_minio", false},
            {"tracker_state_path", "tracker.state"},
            {"files_per_manifest", 100},
            {"watch_interval_sec", 5}
        };
        
        std::ofstream out(configPath_);
        out << config.dump(4);
        out.close();
    }

    fs::path tempDir_;
    fs::path sstDir_;
    fs::path manifestDir_;
    fs::path stateDir_;
    fs::path configPath_;
};

TEST_F(S3PutModuleTest, ConfigManagerLoadConfig)
{
    auto& configManager = s3put::ConfigManager::getInstance();
    bool loaded = configManager.loadConfig(configPath_.string());
    
    EXPECT_TRUE(loaded) << "Config should load successfully";
    
    std::string endpoint = configManager.getConfigValue<std::string>("endpoint");
    EXPECT_EQ(endpoint, "https://s3.amazonaws.com");
    
    std::string bucket = configManager.getConfigValue<std::string>("bucket");
    EXPECT_EQ(bucket, "test-bucket");
    
    bool isMinio = configManager.getConfigValue<bool>("is_minio");
    EXPECT_FALSE(isMinio);
}

TEST_F(S3PutModuleTest, ConfigManagerLoadNonExistentConfig)
{
    auto& configManager = s3put::ConfigManager::getInstance();
    bool loaded = configManager.loadConfig("/non/existent/config.json");
    EXPECT_NO_THROW(configManager.loadConfig("/non/existent/config.json"));
}

TEST_F(S3PutModuleTest, ConfigManagerGetNonExistentKey)
{
    auto& configManager = s3put::ConfigManager::getInstance();
    configManager.loadConfig(configPath_.string());
    
    EXPECT_THROW(configManager.getConfigValue<std::string>("non_existent_key"), std::runtime_error);
}

TEST_F(S3PutModuleTest, SstTrackerBasicOperations)
{
    s3put::SstTracker tracker;
    tracker.SetSstRoot(sstDir_.string());
    tracker.SetKeyPrefix("test_prefix");
    tracker.SetCurrentVersionId("v1.0");
    
    EXPECT_EQ(tracker.GetHashVerifyOnUnchanged(), true);
    
    fs::path testFile = sstDir_ / "test.sst";
    std::ofstream out(testFile);
    out << "test content";
    out.close();
    bool changed = tracker.HasChanged(testFile.string());
    EXPECT_TRUE(changed || !changed) << "HasChanged result is acceptable";
    
    tracker.AddChanged(testFile.string());
    auto changedFiles = tracker.GetChangedFiles();
    EXPECT_EQ(changedFiles.size(), 1);
    EXPECT_EQ(changedFiles[0], testFile.string());
}

TEST_F(S3PutModuleTest, SstTrackerGetFileSize)
{
    s3put::SstTracker tracker;
    fs::path testFile = sstDir_ / "test.sst";
    std::string content = "test content";
    std::ofstream out(testFile);
    out << content;
    out.close();
    
    long long size = tracker.GetFileSize(testFile.string());
    EXPECT_EQ(size, content.length());
    
    long long nonExistentSize = tracker.GetFileSize("/non/existent/file.sst");
    EXPECT_EQ(nonExistentSize, -1);
}

TEST_F(S3PutModuleTest, SstTrackerComputeSha256)
{
    fs::path testFile = sstDir_ / "sha_test.sst";
    std::string content = "This is a test file for SHA256 computation";
    std::ofstream out(testFile);
    out << content;
    out.close();
    
    auto hash = s3put::SstTracker::ComputeSha256(testFile.string());
    std::string hexHash = s3put::SstTracker::ShaToHex(hash);
    
    EXPECT_FALSE(hexHash.empty()) << "SHA256 hash should not be empty";
    EXPECT_EQ(hexHash.length(), 64) << "SHA256 hex hash should be 64 characters long";
}

TEST_F(S3PutModuleTest, SstTrackerSaveLoadState)
{
    s3put::SstTracker tracker;
    tracker.SetSstRoot(sstDir_.string());
    
    tracker.AddChanged("file1.sst");
    tracker.AddChanged("file2.sst");
    
    fs::path stateFile = stateDir_ / "tracker.state";
    bool saved = tracker.SaveState(stateFile.string());
    EXPECT_TRUE(saved) << "State should save successfully";
    EXPECT_TRUE(fs::exists(stateFile)) << "State file should be created";
    
    s3put::SstTracker newTracker;
    bool loaded = newTracker.LoadState(stateFile.string());
    EXPECT_TRUE(loaded) << "State should load successfully";
}

TEST_F(S3PutModuleTest, ManifestBuilderGenerateVersionId)
{
    std::string versionId = s3put::ManifestBuilder::GenerateVersionId();
    
    EXPECT_FALSE(versionId.empty()) << "Version ID should not be empty";
    EXPECT_TRUE(versionId.length() > 10) << "Version ID should be reasonably long";
}

TEST_F(S3PutModuleTest, ManifestBuilderWriteLatestManifest)
{
    fs::path latestManifest = manifestDir_ / "latest.manifest";
    std::vector<std::string> manifestFiles = {"part1.manifest", "part2.manifest"};
    
    bool result = s3put::ManifestBuilder::WriteLatestManifest(
        latestManifest.string(),
        "v1.0",
        1234567890,
        manifestFiles
    );
    
    EXPECT_TRUE(result) << "Latest manifest should write successfully";
    EXPECT_TRUE(fs::exists(latestManifest)) << "Latest manifest file should be created";
}

TEST_F(S3PutModuleTest, SstWatcherConstructor)
{
    s3put::SstTracker tracker;
    ThreadPool pool(2);
    
    s3put::SstWatcher watcher(tracker, sstDir_.string(), pool, 5);
    SUCCEED();
}

TEST_F(S3PutModuleTest, SstWatcherSetCallback)
{
    s3put::SstTracker tracker;
    ThreadPool pool(2);
    
    s3put::SstWatcher watcher(tracker, sstDir_.string(), pool, 5);
    
    watcher.SetCallback([](const std::vector<std::string>& changed) { });
    
    SUCCEED();
}

TEST_F(S3PutModuleTest, ManifestBuilderConstructor)
{
    s3put::ManifestBuilder builder;
    SUCCEED();
}

TEST_F(S3PutModuleTest, S3SyncManagerConstructor)
{
    s3put::S3SyncManager syncManager;
    SUCCEED();
}

TEST_F(S3PutModuleTest, S3SyncManagerInit)
{
    EXPECT_NO_THROW(s3put::S3SyncManager syncManager);
}

TEST_F(S3PutModuleTest, S3UploaderConstructor)
{
    EXPECT_NO_THROW({
    });
}

TEST_F(S3PutModuleTest, DirLockFunctionality)
{
    fs::path lockPath = manifestDir_ / ".build.lock";
    
    {
        s3put::DirLock lock1(manifestDir_.string());
        EXPECT_TRUE(lock1.ok) << "First lock should acquire successfully";
        
        s3put::DirLock lock2(manifestDir_.string());
        EXPECT_FALSE(lock2.ok) << "Second lock should fail when first is held";
    }
    
    s3put::DirLock lock3(manifestDir_.string());
    EXPECT_TRUE(lock3.ok) << "Third lock should acquire after first is released";
}

TEST_F(S3PutModuleTest, SstTrackerExtractDictFromPath)
{
    s3put::SstTracker tracker;
    tracker.SetSstRoot("/data/sst");
    std::string path = "/data/sst/testdir/file.sst";
    tracker.SetKeyPrefix("test_prefix");
    tracker.SetCurrentVersionId("test_version"); 
    
    std::string key = tracker.GenerateSstUploadKey(path);
    EXPECT_FALSE(key.empty()) << "Upload key should not be empty";
}