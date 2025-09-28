#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <string>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include "storage/storage.h"
#include "storage/util.h"
#include "ingest_conf.h"
#include <initializer_list>

using namespace storage;

namespace fs = std::filesystem;

static std::string WriteTempConf(const std::string& content, const std::string& name_hint) {
  fs::path dir = fs::temp_directory_path() / "pikiwi_ingestconf_tests";
  fs::create_directories(dir);
  fs::path p = dir / name_hint;
  std::ofstream ofs(p);
  ofs << content;
  return p.string();
}

static std::string MkTmpDBDir(const std::string& name_hint) {
  fs::path dir = fs::temp_directory_path() / ("pikiwi_ingest_db_" + name_hint);
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir.string();
}

TEST(IngestConfTest, Load_Defaults_WhenEmptyFile) {
  std::string conf_path = WriteTempConf("", "empty_ingest.conf");
  IngestConf conf(conf_path);
  int ret = conf.Load();
  EXPECT_EQ(0, ret);

  auto opt = conf.MakeIngestOptions();
  EXPECT_TRUE(opt.move_files);
  EXPECT_TRUE(opt.verify_checksums_before_ingest);
  EXPECT_TRUE(opt.snapshot_consistency);
  EXPECT_TRUE(opt.allow_blocking_flush);
  EXPECT_FALSE(opt.ingest_behind);
  EXPECT_TRUE(opt.write_global_seqno);
  EXPECT_TRUE(opt.allow_global_seqno);
}

TEST(IngestConfTest, Load_Overrides_And_MakeOptions) {
  const std::string conf_text = R"CONF(
# IngestExternalFileOptions
ingest.options.move-files : false
ingest.options.verify : false
ingest.options.snapshot-consistency : false
ingest.options.allow-blocking-flush : false
ingest.options.ingest-behind : true
ingest.options.write-global-seqno : false
ingest.options.allow-global-seqno : false

# Aggressive
ingest.aggressive.disable-auto-compactions : true
ingest.aggressive.level0-file-num-compaction-trigger : 64
ingest.aggressive.soft-pending-compaction-bytes-limit : 8589934592
ingest.aggressive.hard-pending-compaction-bytes-limit : 17179869184
ingest.aggressive.max_total_wal_size : 134217728

# Restore
ingest.restore.disable-auto-compactions : false
ingest.restore.soft-pending-compaction-bytes-limit : 68719476736
ingest.restore.hard-pending-compaction-bytes-limit : 137438953472
ingest.restore.max_total_wal_size : 268435456
)CONF";

  std::string conf_path = WriteTempConf(conf_text, "override_ingest.conf");
  IngestConf conf(conf_path);
  int ret = conf.Load();
  EXPECT_EQ(0, ret);

  auto opt = conf.MakeIngestOptions();
  EXPECT_FALSE(opt.move_files);
  EXPECT_FALSE(opt.snapshot_consistency);
  EXPECT_FALSE(opt.allow_blocking_flush);
  EXPECT_TRUE(opt.ingest_behind);
  EXPECT_FALSE(opt.write_global_seqno);
  EXPECT_FALSE(opt.allow_global_seqno);
}

TEST(IngestConfTest, Apply_Aggressive_Then_Restore_On_LiveDB) {
  std::string dbdir = MkTmpDBDir("apply_opts");
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::DB* db = nullptr;
  auto st_open = rocksdb::DB::Open(options, dbdir, &db);
  ASSERT_TRUE(st_open.ok()) << st_open.ToString();
  std::unique_ptr<rocksdb::DB> db_guard(db);
  auto* cf = db->DefaultColumnFamily();

  const std::string conf_text = R"CONF(
ingest.aggressive.disable-auto-compactions : true
ingest.aggressive.level0-file-num-compaction-trigger : 32
ingest.aggressive.soft-pending-compaction-bytes-limit : 4294967296
ingest.aggressive.hard-pending-compaction-bytes-limit : 8589934592
ingest.aggressive.max_total_wal_size : 134217728

ingest.restore.disable-auto-compactions : false
ingest.restore.soft-pending-compaction-bytes-limit : 68719476736
ingest.restore.hard-pending-compaction-bytes-limit : 137438953472
ingest.restore.max_total_wal_size : 268435456

ingest.options.move-files : true
ingest.options.verify : true
ingest.options.snapshot-consistency : true
ingest.options.allow-blocking-flush : true
ingest.options.ingest-behind : false
)CONF";

  std::string conf_path = WriteTempConf(conf_text, "apply_opts_ingest.conf");
  IngestConf conf(conf_path);
  int ret = conf.Load();
  ASSERT_EQ(0, ret);

  auto st1 = conf.ApplyAggressiveOptions(db, cf);
  EXPECT_TRUE(st1.ok()) << st1.ToString();

  auto wst = db->Put(rocksdb::WriteOptions(), "k", "v");
  EXPECT_TRUE(wst.ok());

  auto st2 = conf.ApplyRestoreOptions(db, cf);
  EXPECT_TRUE(st2.ok()) << st2.ToString();

  int rewrite_code = conf.ConfigRewrite();
  EXPECT_EQ(0, rewrite_code);
}

TEST(IngestConfTest, Load_InvalidBoolean) {
  const std::string conf_text = R"CONF(
ingest.options.move-files : notabool
)CONF";
  std::string conf_path = WriteTempConf(conf_text, "invalid_bool.conf");
  IngestConf conf(conf_path);
  int ret = conf.Load();
  EXPECT_EQ(0, ret);

  auto opt = conf.MakeIngestOptions();
  (void)opt; 
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}