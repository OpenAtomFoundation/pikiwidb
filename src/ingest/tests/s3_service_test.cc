#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

#include "ingest_s3_service.h"  

namespace fs = std::filesystem;

static std::string WriteTempJson(const std::string& content, const std::string& name_hint) {
  fs::path dir = fs::temp_directory_path() / "pikiwi_s3service_tests";
  fs::create_directories(dir);
  fs::path p = dir / name_hint;
  std::ofstream ofs(p);
  ofs << content;
  return p.string();
}

TEST(S3ServiceTest, Start_WithMissingFile_ShouldFailAndFillErr) {
  S3Service s;
  std::string err;
  bool ok = s.Start("/definitely/not/exist/s3.conf", &err);
  EXPECT_FALSE(ok);
  EXPECT_FALSE(err.empty()); 
}

TEST(S3ServiceTest, Start_WithBadJson_ShouldFail) {
  const std::string bad_json = R"JSON(
    {  "endpoint": "http://127.0.0.1:9000",   // <-- 注释会导致 parse 抛异常（已开启严格模式）
       "bucket": "bk"
  )JSON";
  std::string path = WriteTempJson(bad_json, "bad_s3.json");

  S3Service s;
  std::string err;
  bool ok = s.Start(path, &err);
  EXPECT_FALSE(ok);
  EXPECT_FALSE(err.empty());
}

TEST(S3ServiceTest, Start_WithMinimalValidJson_ShouldSucceedAndExposeRuntimeParams) {
  const std::string good_json = R"JSON(
  {
    "region": "us-east-1",
    "endpoint": "http://127.0.0.1:9000",
    "ak": "minio",
    "sk": "minio123",
    "bucket": "test-bucket",
    "transfer_threads": 2,
    "transfer_buf_bytes": 1048576,
    "max_inflight": 4,
    "retry_max_attempts": 5,
    "retry_base_ms": 10,
    "retry_max_ms": 500,
    "retry_jitter": 0.1
  }
  )JSON";
  std::string path = WriteTempJson(good_json, "ok_s3.json");

  S3Service s;
  std::string err;
  bool ok = s.Start(path, &err);
  ASSERT_TRUE(ok) << err;

  auto cli = s.Client();
  auto tm  = s.TransferMgr();
  ASSERT_TRUE(cli != nullptr);
  ASSERT_TRUE(tm  != nullptr);

  EXPECT_EQ(s.Bucket(), "test-bucket");
  EXPECT_EQ(s.TransferThreads(), 2);
  EXPECT_EQ(s.TransferBufBytes(), static_cast<size_t>(1048576));
  EXPECT_EQ(s.MaxInflight(), static_cast<size_t>(4));
  EXPECT_EQ(s.RetryMaxAttempts(), 5);
  EXPECT_EQ(s.RetryBaseMs(), 10);
  EXPECT_EQ(s.RetryMaxMs(), 500);
  EXPECT_NEAR(s.RetryJitter(), 0.1, 1e-9);

  s.Stop();
  s.Stop();
}

TEST(S3ServiceTest, Start_WithDefaults_WhenFieldsMissing_ShouldStillWorkAndUseDefaults) {
  const std::string partial_json = R"JSON(
  {
    "endpoint": "http://127.0.0.1:9000",
    "ak": "minio",
    "sk": "minio123",
    "bucket": "bk"
  }
  )JSON";
  std::string path = WriteTempJson(partial_json, "partial_s3.json");

  S3Service s;
  std::string err;
  bool ok = s.Start(path, &err);
  ASSERT_TRUE(ok) << err;

  ASSERT_TRUE(s.Client() != nullptr);
  ASSERT_TRUE(s.TransferMgr() != nullptr);

  EXPECT_EQ(s.Bucket(), "bk");
  EXPECT_EQ(s.TransferThreads(), 8);
  EXPECT_EQ(s.TransferBufBytes(), static_cast<size_t>(8u << 20));
  EXPECT_EQ(s.MaxInflight(), static_cast<size_t>(8));
  EXPECT_EQ(s.RetryMaxAttempts(), 3);
  EXPECT_EQ(s.RetryBaseMs(), 50);
  EXPECT_EQ(s.RetryMaxMs(), 2000);
  EXPECT_NEAR(s.RetryJitter(), 0.2, 1e-9);

  s.Stop();
}

TEST(S3ServiceTest, Restart_ShouldRecreateResourcesAndNotLeak) {
  const std::string good_json = R"JSON(
  {
    "endpoint": "http://127.0.0.1:9000",
    "ak": "minio",
    "sk": "minio123",
    "bucket": "bk"
  }
  )JSON";
  std::string path = WriteTempJson(good_json, "restart_s3.json");

  S3Service s;
  std::string err;
  ASSERT_TRUE(s.Start(path, &err)) << err;

  auto c1 = s.Client();
  auto t1 = s.TransferMgr();
  ASSERT_TRUE(c1 && t1);

  s.Stop(); 
  ASSERT_TRUE(s.Start(path, &err)) << err; 

  auto c2 = s.Client();
  auto t2 = s.TransferMgr();
  ASSERT_TRUE(c2 && t2);

  s.Stop();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}