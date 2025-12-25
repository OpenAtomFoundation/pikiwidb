// Copyright (c) 2025-present, PikiwiDB Project
// Licensed under the BSD-style license found in the LICENSE file in the root directory of this source tree.
// This source code is also available under the terms of the GNU General Public License, version 3.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <ctime>
#include <sys/stat.h>
#include <getopt.h>

#include "storage/include/storage/storage.h"
#include "rocksdb/options.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "storage/src/base_data_key_format.h"
#include "storage/src/base_meta_value_format.h"
#include "storage/src/lists_meta_value_format.h"
#include "storage/src/custom_comparator.h"

// Utility function to check if a directory exists
bool DirectoryExists(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// Replace special characters for consistent display
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
  }
  return str;
}

// Print usage information
void PrintUsage() {
  std::cout << "Usage: bigkey_analyzer [OPTIONS] <db_path>" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --min-size=SIZE       Only show keys larger than SIZE bytes" << std::endl;
  std::cout << "  --top=N               Only show top N largest keys" << std::endl;
  std::cout << "  --prefix-stat         Show statistics by key prefix" << std::endl;
  std::cout << "  --prefix-delimiter=C  Character used to delimit prefix (default: ':')" << std::endl;
  std::cout << "  --type=TYPE           Only analyze specific type (strings|hashes|lists|sets|zsets|all)" << std::endl;
  std::cout << "  --output=FILE         Write output to file instead of stdout" << std::endl;
  std::cout << "  --help                Display this help message" << std::endl;
}

// Data structure to hold key information
struct KeyInfo {
  std::string type;
  std::string key;
  int64_t size;
  int64_t ttl;
  
  // Default constructor
  KeyInfo() : type(""), key(""), size(0), ttl(-1) {}
  
  // Constructor with const references
  KeyInfo(const std::string& t, const std::string& k, int64_t s, int64_t tt)
    : type(t), key(k), size(s), ttl(tt) {}
  
  // Constructor with rvalue references for better move semantics
  KeyInfo(std::string&& t, std::string&& k, int64_t s, int64_t tt)
    : type(std::move(t)), key(std::move(k)), size(s), ttl(tt) {}
  
  // Mixed constructor (const char* literals are common)
  KeyInfo(const char* t, const std::string& k, int64_t s, int64_t tt)
    : type(t), key(k), size(s), ttl(tt) {}
  
  KeyInfo(const char* t, std::string&& k, int64_t s, int64_t tt)
    : type(t), key(std::move(k)), size(s), ttl(tt) {}
    
  bool operator<(const KeyInfo& other) const {
    return size > other.size; // Sort in descending order by size
  }
};

// Data structure for prefix statistics
struct PrefixStat {
  size_t count = 0;
  int64_t total_size = 0;
  
  void Add(int64_t size) {
    count++;
    total_size += size;
  }
};

// Configuration for the analyzer
struct Config {
  std::string db_path;
  int64_t min_size = 0;
  int top_n = -1;
  bool prefix_stat = false;
  std::string prefix_delimiter = ":";
  std::string type_filter = "all";
  std::string output_file;
};

// Parse command line arguments
bool ParseArgs(int argc, char* argv[], Config& config) {
  if (argc < 2) {
    PrintUsage();
    return false;
  }
  
  static struct option long_options[] = {
    {"min-size", required_argument, 0, 'm'},
    {"top", required_argument, 0, 't'},
    {"prefix-stat", no_argument, 0, 'p'},
    {"prefix-delimiter", required_argument, 0, 'd'},
    {"type", required_argument, 0, 'y'},
    {"output", required_argument, 0, 'o'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };
  
  int opt;
  int option_index = 0;
  
  // Set default values
  config.min_size = 0;
  config.top_n = -1;
  config.prefix_stat = false;
  config.prefix_delimiter = ":";
  config.type_filter = "all";
  
  while ((opt = getopt_long(argc, argv, "m:t:pd:y:o:h", long_options, &option_index)) != -1) {
    switch (opt) {
      case 'm':
        config.min_size = std::stoll(optarg);
        break;
      case 't':
        config.top_n = std::stoi(optarg);
        break;
      case 'p':
        config.prefix_stat = true;
        break;
      case 'd':
        config.prefix_delimiter = optarg;
        break;
      case 'y':
        config.type_filter = optarg;
        break;
      case 'o':
        config.output_file = optarg;
        break;
      case 'h':
        PrintUsage();
        return false;
      default:
        PrintUsage();
        return false;
    }
  }
  
  if (optind >= argc) {
    std::cerr << "Error: Missing database path" << std::endl;
    PrintUsage();
    return false;
  }
  
  config.db_path = argv[optind];
  
  // Validate the database path
  if (!DirectoryExists(config.db_path)) {
    std::cerr << "Error: Database directory does not exist: " << config.db_path << std::endl;
    return false;
  }
  
  return true;
}

// Analyze strings database
void AnalyzeStrings(const std::string& path, std::vector<KeyInfo>& key_infos, const Config& config) {
  if (!DirectoryExists(path)) {
    std::cerr << "Skipping strings: directory not found: " << path << std::endl;
    return;
  }
  
  std::cout << "Analyzing strings database at " << path << "..." << std::endl;
  
  rocksdb::Options options;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, path, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening strings database: " << status.ToString() << std::endl;
    return;
  }
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  
  rocksdb::ReadOptions read_options;
  auto iter = db->NewIterator(read_options);
  
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    const std::string& key = iter->key().ToString();
    const std::string& value = iter->value().ToString();
    
    // Extract timestamp from value (if exists)
    int32_t ttl = -1;
    int64_t ts = 0;
    
    if (value.size() >= 12) {  // At least has timestamp
      // The format in storage engine may be different from the old one
      // We're simplifying here - real implementation would use ParsedStringsValue
      ts = *reinterpret_cast<const int64_t*>(value.data() + value.size() - 12);
      if (ts != 0) {
        int64_t diff = ts - curtime;
        ttl = diff > 0 ? diff : -1;
      }
    }
    
    int64_t size = key.size() + value.size();
    
    if (size >= config.min_size) {
      std::string display_key = ReplaceAll(key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("string", display_key, size, ttl);
    }
  }
  
  delete iter;
  delete db;
}

// Analyze hashes database
void AnalyzeHashes(const std::string& path, std::vector<KeyInfo>& key_infos, const Config& config) {
  if (!DirectoryExists(path)) {
    std::cerr << "Skipping hashes: directory not found: " << path << std::endl;
    return;
  }
  
  std::cout << "Analyzing hashes database at " << path << "..." << std::endl;
  
  // Open database with column families
  rocksdb::DBOptions db_options;
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());
  column_families.emplace_back("data_cf", rocksdb::ColumnFamilyOptions());
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(db_options, path, column_families, &handles, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening hashes database: " << status.ToString() << std::endl;
    return;
  }
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  
  rocksdb::ReadOptions read_options;
  
  // Using an unordered_map to group hash fields by key
  std::unordered_map<std::string, std::pair<int64_t, int64_t>> hash_sizes; // key -> (size, ttl)
  
  // Read metadata from default column family (handles[0])
  auto meta_iter = db->NewIterator(read_options, handles[0]);
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    rocksdb::Slice key_slice = meta_iter->key();
    rocksdb::Slice value_slice = meta_iter->value();
    std::string key = key_slice.ToString();
    
    int64_t ttl = -1;
    
    // Parse metadata value to get TTL and check if stale
    if (value_slice.size() >= storage::ParsedBaseMetaValue::kBaseMetaValueSuffixLength) {
      storage::ParsedHashesMetaValue parsed_meta(value_slice);
      
      // Skip stale or empty hashes
      if (parsed_meta.IsStale() || parsed_meta.count() == 0) {
        continue;
      }
      
      int32_t timestamp = parsed_meta.timestamp();
      if (timestamp > 0 && !parsed_meta.IsPermanentSurvival()) {
        int64_t diff = timestamp - curtime;
        ttl = diff > 0 ? diff : -2;
      }
    }
    
    // Initialize with base metadata size (key + 12 bytes overhead)
    int64_t sum = key.size() + 12;
    hash_sizes[key] = std::make_pair(sum, ttl);
  }
  delete meta_iter;
  
  // Read data fields from data column family (handles[1])
  auto data_iter = db->NewIterator(read_options, handles[1]);
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    rocksdb::Slice encoded_key_slice = data_iter->key();
    rocksdb::Slice value_slice = data_iter->value();
    
    // Parse the data key to extract the hash key and field
    try {
      storage::ParsedHashesDataKey parsed_key(encoded_key_slice);
      std::string hash_key = parsed_key.key().ToString();
      std::string field = parsed_key.field().ToString();
      
      // Calculate field size: 4 (size prefix) + key + 4 (size prefix) + field + value
      int64_t field_size = 4 + hash_key.size() + 4 + field.size() + value_slice.size();
      
      // Add field size to the corresponding hash
      auto it = hash_sizes.find(hash_key);
      if (it != hash_sizes.end()) {
        it->second.first += field_size;
      } else {
        // If metadata not found, initialize with field size and default ttl
        hash_sizes[hash_key] = std::make_pair(hash_key.size() + 12 + field_size, -1);
      }
    } catch (...) {
      // Skip malformed keys
      continue;
    }
  }
  delete data_iter;
  
  // Add hash keys to the result
  for (const auto& entry : hash_sizes) {
    if (entry.second.first >= config.min_size) {
      std::string display_key = ReplaceAll(entry.first, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("hash", display_key, entry.second.first, entry.second.second);
    }
  }
  
  // Cleanup
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

// Analyze sets database
void AnalyzeSets(const std::string& path, std::vector<KeyInfo>& key_infos, const Config& config) {
  if (!DirectoryExists(path)) {
    std::cerr << "Skipping sets: directory not found: " << path << std::endl;
    return;
  }
  
  std::cout << "Analyzing sets database at " << path << "..." << std::endl;
  
  // Open database with column families
  rocksdb::DBOptions db_options;
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());
  column_families.emplace_back("member_cf", rocksdb::ColumnFamilyOptions());
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(db_options, path, column_families, &handles, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening sets database: " << status.ToString() << std::endl;
    return;
  }
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  
  rocksdb::ReadOptions read_options;
  
  // Using an unordered_map to group set members by key
  std::unordered_map<std::string, std::pair<int64_t, int64_t>> set_sizes; // key -> (size, ttl)
  
  // Read metadata from default column family (handles[0])
  auto meta_iter = db->NewIterator(read_options, handles[0]);
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    rocksdb::Slice key_slice = meta_iter->key();
    rocksdb::Slice value_slice = meta_iter->value();
    std::string key = key_slice.ToString();
    
    int64_t ttl = -1;
    
    // Parse metadata value to get TTL
    if (value_slice.size() >= storage::ParsedBaseMetaValue::kBaseMetaValueSuffixLength) {
      storage::ParsedSetsMetaValue parsed_meta(value_slice);
      
      // Skip stale or empty sets
      if (parsed_meta.IsStale() || parsed_meta.count() == 0) {
        continue;
      }
      
      int32_t timestamp = parsed_meta.timestamp();
      if (timestamp > 0 && !parsed_meta.IsPermanentSurvival()) {
        int64_t diff = timestamp - curtime;
        ttl = diff > 0 ? diff : -2;
      }
    }
    
    // Initialize with base metadata size (key + 12 bytes overhead)
    int64_t sum = key.size() + 12;
    set_sizes[key] = std::make_pair(sum, ttl);
  }
  delete meta_iter;
  
  // Read data members from data column family (handles[1])
  auto data_iter = db->NewIterator(read_options, handles[1]);
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    rocksdb::Slice encoded_key_slice = data_iter->key();
    rocksdb::Slice value_slice = data_iter->value();
    
    // Parse the data key to extract the set key and member
    try {
      storage::ParsedSetsMemberKey parsed_key(encoded_key_slice);
      std::string set_key = parsed_key.key().ToString();
      std::string member = parsed_key.member().ToString();
      
      // Calculate member size: 4 (size prefix) + key + 4 (size prefix) + member
      int64_t member_size = 4 + set_key.size() + 4 + member.size();
      
      // Add member size to the corresponding set
      auto it = set_sizes.find(set_key);
      if (it != set_sizes.end()) {
        it->second.first += member_size;
      } else {
        // If metadata not found, initialize with member size and default ttl
        set_sizes[set_key] = std::make_pair(set_key.size() + 12 + member_size, -1);
      }
    } catch (...) {
      // Skip malformed keys
      continue;
    }
  }
  delete data_iter;
  
  // Add set keys to the result
  for (const auto& entry : set_sizes) {
    if (entry.second.first >= config.min_size) {
      std::string display_key = ReplaceAll(entry.first, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("set", display_key, entry.second.first, entry.second.second);
    }
  }
  
  // Cleanup
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

// Analyze zsets database
void AnalyzeZsets(const std::string& path, std::vector<KeyInfo>& key_infos, const Config& config) {
  if (!DirectoryExists(path)) {
    std::cerr << "Skipping zsets: directory not found: " << path << std::endl;
    return;
  }
  
  std::cout << "Analyzing zsets database at " << path << "..." << std::endl;
  
  // Open database with column families
  std::vector<std::string> column_families;
  rocksdb::Options options;
  // 先列出所有可用的列族
  rocksdb::Status s = rocksdb::DB::ListColumnFamilies(options, path, &column_families);
  
  if (!s.ok()) {
    std::cerr << "Error listing column families for zsets: " << s.ToString() << std::endl;
    return;
  }
  
  rocksdb::DBOptions db_options;
  std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
  
  // 添加所有列族到描述符
  for (const auto& cf_name : column_families) {
    cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions());
  }
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(db_options, path, cf_descriptors, &handles, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening zsets database: " << status.ToString() << std::endl;
    return;
  }
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  
  rocksdb::ReadOptions read_options;
  
  // Using an unordered_map to group zset members by key
  std::unordered_map<std::string, std::pair<int64_t, int64_t>> zset_sizes; // key -> (size, ttl)
  
  // 找到default、data_cf和score_cf的索引
  int default_cf_index = -1;
  int data_cf_index = -1;
  int score_cf_index = -1;
  
  for (size_t i = 0; i < column_families.size(); i++) {
    if (column_families[i] == "default") {
      default_cf_index = i;
    } else if (column_families[i] == "data_cf") {
      data_cf_index = i;
    } else if (column_families[i] == "score_cf") {
      score_cf_index = i;
    }
  }
  
  // 处理元数据 (default 列族)
  if (default_cf_index != -1) {
    auto meta_iter = db->NewIterator(read_options, handles[default_cf_index]);
    for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
      rocksdb::Slice key_slice = meta_iter->key();
      rocksdb::Slice value_slice = meta_iter->value();
      std::string key = key_slice.ToString();
      
      int64_t ttl = -1;
      
      // Parse metadata value to get TTL
      if (value_slice.size() >= storage::ParsedBaseMetaValue::kBaseMetaValueSuffixLength) {
        storage::ParsedZSetsMetaValue parsed_meta(value_slice);
        
        // Skip stale or empty zsets
        if (parsed_meta.IsStale() || parsed_meta.count() == 0) {
          continue;
        }
        
        int32_t timestamp = parsed_meta.timestamp();
        if (timestamp > 0 && !parsed_meta.IsPermanentSurvival()) {
          int64_t diff = timestamp - curtime;
          ttl = diff > 0 ? diff : -2;
        }
      }
      
      // Initialize with base metadata size (key + 12 bytes overhead)
      int64_t sum = key.size() + 12;
      zset_sizes[key] = std::make_pair(sum, ttl);
    }
    delete meta_iter;
  }
  
  // 处理成员数据 (data_cf 列族)
  if (data_cf_index != -1) {
    auto data_iter = db->NewIterator(read_options, handles[data_cf_index]);
    for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
      rocksdb::Slice encoded_key_slice = data_iter->key();
      rocksdb::Slice value_slice = data_iter->value();
      
      // Parse the data key to extract the zset key and member
      try {
        storage::ParsedZSetsMemberKey parsed_key(encoded_key_slice);
        std::string zset_key = parsed_key.key().ToString();
        std::string member = parsed_key.member().ToString();
        
        // Calculate member size: 4 + key + 4 + member + value (score)
        int64_t member_size = 4 + zset_key.size() + 4 + member.size() + value_slice.size();
        
        // Add member size to the corresponding zset
        auto it = zset_sizes.find(zset_key);
        if (it != zset_sizes.end()) {
          it->second.first += member_size;
        } else {
          // If metadata not found, initialize with member size and default ttl
          zset_sizes[zset_key] = std::make_pair(zset_key.size() + 12 + member_size, -1);
        }
      } catch (...) {
        // Skip malformed keys
        continue;
      }
    }
    delete data_iter;
  }
  
  // 处理分数数据 (score_cf 列族)
  if (score_cf_index != -1) {
    auto score_iter = db->NewIterator(read_options, handles[score_cf_index]);
    for (score_iter->SeekToFirst(); score_iter->Valid(); score_iter->Next()) {
      rocksdb::Slice encoded_key_slice = score_iter->key();
      rocksdb::Slice value_slice = score_iter->value();
      
      // ZSetsScoreKey has the same structure as other data keys
      try {
        // 这里是分数数据，需要计算额外的开销
        std::string zset_key;
        std::string member;
        
        // 尝试解析，这里可能不是完全准确
        // 但我们至少要计算正确的大小
        try {
          storage::ParsedZSetsMemberKey parsed_key(encoded_key_slice);
          zset_key = parsed_key.key().ToString();
          member = parsed_key.member().ToString();
        } catch (...) {
          // 如果上面的解析失败，使用通用方法提取
          size_t pos = encoded_key_slice.ToString().find_first_of('\0');
          if (pos != std::string::npos && pos > 0) {
            zset_key = encoded_key_slice.ToString().substr(0, pos);
          } else {
            continue; // 跳过无法解析的键
          }
        }
        
        // 计算score entry大小：4 + key + 8 (score) + 4 + member
        int64_t score_size = 4 + zset_key.size() + 8 + 4 + (member.empty() ? 8 : member.size());
        
        // 添加分数条目大小到对应的zset
        auto it = zset_sizes.find(zset_key);
        if (it != zset_sizes.end()) {
          it->second.first += score_size;
        }
      } catch (...) {
        // 跳过畸形键
        continue;
      }
    }
    delete score_iter;
  }
  
  // Add zset keys to the result
  for (const auto& entry : zset_sizes) {
    if (entry.second.first >= config.min_size) {
      std::string display_key = ReplaceAll(entry.first, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("zset", display_key, entry.second.first, entry.second.second);
    }
  }
  
  // Cleanup
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

// Analyze lists database
void AnalyzeLists(const std::string& path, std::vector<KeyInfo>& key_infos, const Config& config) {
  if (!DirectoryExists(path)) {
    std::cerr << "Skipping lists: directory not found: " << path << std::endl;
    return;
  }
  
  std::cout << "Analyzing lists database at " << path << "..." << std::endl;
  
  // Open database with column families
  rocksdb::DBOptions db_options;
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());
  column_families.emplace_back("data_cf", rocksdb::ColumnFamilyOptions());
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(db_options, path, column_families, &handles, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening lists database: " << status.ToString() << std::endl;
    return;
  }
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  
  rocksdb::ReadOptions read_options;
  
  // Using an unordered_map to group list items by key
  std::unordered_map<std::string, std::pair<int64_t, int64_t>> list_sizes; // key -> (size, ttl)
  
  // Read metadata from default column family (handles[0])
  auto meta_iter = db->NewIterator(read_options, handles[0]);
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    rocksdb::Slice key_slice = meta_iter->key();
    rocksdb::Slice value_slice = meta_iter->value();
    std::string key = key_slice.ToString();
    
    int64_t ttl = -1;
    
    // Parse metadata value to get TTL
    if (value_slice.size() >= storage::ParsedBaseMetaValue::kBaseMetaValueSuffixLength) {
      storage::ParsedListsMetaValue parsed_meta(value_slice);
      
      // Skip empty lists
      if (parsed_meta.count() == 0) {
        continue;
      }
      
      int32_t timestamp = parsed_meta.timestamp();
      if (timestamp > 0) {
        int64_t diff = timestamp - curtime;
        ttl = diff > 0 ? diff : -2;
      }
    }
    
    // Initialize with base metadata size (key + 12 + 16 bytes overhead)
    int64_t sum = key.size() + 12 + 16;
    list_sizes[key] = std::make_pair(sum, ttl);
  }
  delete meta_iter;
  
  // Read data items from data column family (handles[1])
  auto data_iter = db->NewIterator(read_options, handles[1]);
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    rocksdb::Slice encoded_key_slice = data_iter->key();
    rocksdb::Slice value_slice = data_iter->value();
    
    // Parse the data key to extract the list key
    try {
      storage::ParsedBaseDataKey parsed_key(encoded_key_slice); // Lists use BaseDataKey directly
      std::string list_key = parsed_key.key().ToString();
      
      // Calculate element size: 4 + key + 4 + 8 (index) + element
      int64_t element_size = 4 + list_key.size() + 4 + 8 + value_slice.size();
      
      // Add element size to the corresponding list
      auto it = list_sizes.find(list_key);
      if (it != list_sizes.end()) {
        it->second.first += element_size;
      } else {
        // If metadata not found, initialize with element size and default ttl
        list_sizes[list_key] = std::make_pair(list_key.size() + 12 + 16 + element_size, -1);
      }
    } catch (...) {
      // Skip malformed keys
      continue;
    }
  }
  delete data_iter;
  
  // Add list keys to the result
  for (const auto& entry : list_sizes) {
    if (entry.second.first >= config.min_size) {
      std::string display_key = ReplaceAll(entry.first, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("list", display_key, entry.second.first, entry.second.second);
    }
  }
  
  // Cleanup
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

// Get the prefix of a key
std::string GetKeyPrefix(const std::string& key, const std::string& delimiter) {
  size_t pos = key.find(delimiter);
  if (pos != std::string::npos) {
    return key.substr(0, pos);
  }
  return key;  // Return the entire key if no delimiter found
}

// Generate prefix statistics
void GeneratePrefixStats(const std::vector<KeyInfo>& key_infos, const std::string& delimiter, std::ostream& out) {
  std::unordered_map<std::string, PrefixStat> prefix_stats;
  
  for (const auto& info : key_infos) {
    std::string prefix = GetKeyPrefix(info.key, delimiter);
    prefix_stats[prefix].Add(info.size);
  }
  
  // Convert to vector for sorting
  std::vector<std::pair<std::string, PrefixStat>> sorted_stats;
  for (const auto& entry : prefix_stats) {
    sorted_stats.emplace_back(entry);
  }
  
  // Sort by total size in descending order
  std::sort(sorted_stats.begin(), sorted_stats.end(), 
    [](const auto& a, const auto& b) {
      return a.second.total_size > b.second.total_size;
    });
  
  // Output header
  out << "\n===== Key Prefix Statistics =====\n";
  out << "Prefix\tCount\tTotal Size\tAvg Size\n";
  
  // Output stats
  for (const auto& entry : sorted_stats) {
    double avg_size = static_cast<double>(entry.second.total_size) / entry.second.count;
    out << entry.first << "\t" 
        << entry.second.count << "\t" 
        << entry.second.total_size << "\t"
        << avg_size << "\n";
  }
}

int main(int argc, char *argv[]){
  // Parse command line arguments
  Config config;
  if (!ParseArgs(argc, argv, config)) {
    return 1;
  }
  
  // Vector to store key information
  std::vector<KeyInfo> key_infos;
  
  // Create output stream
  std::unique_ptr<std::ofstream> file_out;
  std::ostream* out = &std::cout;
  
  if (!config.output_file.empty()) {
    file_out = std::make_unique<std::ofstream>(config.output_file);
    if (!file_out->is_open()) {
      std::cerr << "Error opening output file: " << config.output_file << std::endl;
      return 1;
    }
    out = file_out.get();
  }
  
  // Analyze each database type
  if (config.type_filter == "all" || config.type_filter == "strings") {
    std::string path = config.db_path + "/strings";
    AnalyzeStrings(path, key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "hashes") {
    std::string path = config.db_path + "/hashes";
    AnalyzeHashes(path, key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "sets") {
    std::string path = config.db_path + "/sets";
    AnalyzeSets(path, key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "zsets") {
    std::string path = config.db_path + "/zsets";
    AnalyzeZsets(path, key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "lists") {
    std::string path = config.db_path + "/lists";
    AnalyzeLists(path, key_infos, config);
  }
  
  // Sort keys by size (largest first)
  std::sort(key_infos.begin(), key_infos.end());
  
  // Limit to top N if requested
  if (config.top_n > 0 && config.top_n < static_cast<int>(key_infos.size())) {
    key_infos.resize(config.top_n);
  }
  
  // Output results
  *out << "===== Big Key Analysis =====\n";
  *out << "Type\tSize\tKey\tTTL\n";
  
  for (const auto& info : key_infos) {
    *out << info.type << " " << info.size << " " << info.key << " " << info.ttl << "\n";
  }
  
  // Generate prefix statistics if requested
  if (config.prefix_stat) {
    GeneratePrefixStats(key_infos, config.prefix_delimiter, *out);
  }
  
  // Output summary
  *out << "\n===== Summary =====\n";
  *out << "Total keys analyzed: " << key_infos.size() << "\n";
  
  // Count by type
  std::unordered_map<std::string, int> type_counts;
  std::unordered_map<std::string, int64_t> type_sizes;
  
  for (const auto& info : key_infos) {
    type_counts[info.type]++;
    type_sizes[info.type] += info.size;
  }
  
  *out << "Keys by type:\n";
  for (const auto& entry : type_counts) {
    double avg_size = static_cast<double>(type_sizes[entry.first]) / entry.second;
    double mb_size = static_cast<double>(type_sizes[entry.first]) / (1024 * 1024);
    
    *out << "  " << entry.first << ": " << entry.second << " keys, "
         << mb_size << " MB total, "
         << avg_size << " bytes avg\n";
  }
  
  return 0;
}
