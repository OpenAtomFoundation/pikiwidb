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

#include "rocksdb/options.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"

#include "storage/storage_define.h"
#include "src/storage/src/base_value_format.h"
#include "src/storage/src/base_meta_value_format.h"
#include "src/storage/src/strings_value_format.h"
#include "src/storage/src/base_data_value_format.h"
#include "src/storage/src/lists_meta_value_format.h"
#include "src/storage/src/lists_data_key_format.h"
#include "src/storage/src/zsets_data_key_format.h"
#include "src/storage/src/coding.h"

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
    start_pos += to.length();
  }
  return str;
}

// Decode user key from encoded key
std::string DecodeUserKey(const rocksdb::Slice& encoded_key) {
  std::string user_key;
  storage::DecodeUserKey(encoded_key.data(), encoded_key.size(), &user_key);
  return user_key;
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
  
  KeyInfo() : type(""), key(""), size(0), ttl(-1) {}
  
  KeyInfo(const std::string& t, const std::string& k, int64_t s, int64_t tt)
    : type(t), key(k), size(s), ttl(tt) {}
  
  KeyInfo(std::string&& t, std::string&& k, int64_t s, int64_t tt)
    : type(std::move(t)), key(std::move(k)), size(s), ttl(tt) {}
  
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
  
  if (!DirectoryExists(config.db_path)) {
    std::cerr << "Error: Database directory does not exist: " << config.db_path << std::endl;
    return false;
  }
  
  return true;
}

// Analyze strings in MetaCF
void AnalyzeStrings(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* meta_handle,
                   std::vector<KeyInfo>& key_infos, const Config& config) {
  std::cout << "Analyzing strings..." << std::endl;
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  curtime *= 1000; // Convert to milliseconds
  
  rocksdb::ReadOptions read_options;
  std::unique_ptr<rocksdb::Iterator> iter(db->NewIterator(read_options, meta_handle));
  
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    const rocksdb::Slice& encoded_key = iter->key();
    const rocksdb::Slice& value = iter->value();
    
    // Check if this is a string type
    if (value.size() < 1) continue;
    
    storage::DataType type = static_cast<storage::DataType>(static_cast<uint8_t>(value[0]));
    if (type != storage::DataType::kStrings) {
      continue;
    }
    
    // Decode the user key
    std::string user_key = DecodeUserKey(encoded_key);
    
    // Parse the value
    std::string value_str = value.ToString();
    storage::ParsedStringsValue parsed_value(&value_str);
    
    // Calculate TTL
    int64_t ttl = -1;
    if (!parsed_value.IsPermanentSurvival()) {
      int64_t etime = parsed_value.Etime();
      if (etime > curtime) {
        ttl = (etime - curtime) / 1000; // Convert to seconds
      }
    }
    
    // Skip if expired
    if (parsed_value.IsStale()) {
      continue;
    }
    
    int64_t size = encoded_key.size() + value.size();
    
    if (size >= config.min_size) {
      std::string display_key = ReplaceAll(user_key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("string", std::move(display_key), size, ttl);
    }
  }
  
  if (!iter->status().ok()) {
    std::cerr << "Error iterating strings: " << iter->status().ToString() << std::endl;
  }
}

// Analyze hashes
void AnalyzeHashes(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* meta_handle,
                  rocksdb::ColumnFamilyHandle* data_handle,
                  std::vector<KeyInfo>& key_infos, const Config& config) {
  std::cout << "Analyzing hashes..." << std::endl;
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  curtime *= 1000; // Convert to milliseconds
  
  rocksdb::ReadOptions read_options;
  std::unique_ptr<rocksdb::Iterator> meta_iter(db->NewIterator(read_options, meta_handle));
  
  // Map to store hash sizes: encoded_key -> (size, ttl, version)
  std::unordered_map<std::string, std::tuple<int64_t, int64_t, uint64_t>> hash_info;
  
  // First pass: scan metadata
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    const rocksdb::Slice& encoded_key = meta_iter->key();
    const rocksdb::Slice& value = meta_iter->value();
    
    if (value.size() < 1) continue;
    
    storage::DataType type = static_cast<storage::DataType>(static_cast<uint8_t>(value[0]));
    if (type != storage::DataType::kHashes) {
      continue;
    }
    
    std::string value_str = value.ToString();
    storage::ParsedHashesMetaValue parsed_meta(&value_str);
    
    // Skip if expired or empty
    if (parsed_meta.IsStale() || parsed_meta.Count() == 0) {
      continue;
    }
    
    // Calculate TTL
    int64_t ttl = -1;
    if (!parsed_meta.IsPermanentSurvival()) {
      int64_t etime = parsed_meta.Etime();
      if (etime > curtime) {
        ttl = (etime - curtime) / 1000;
      }
    }
    
    int64_t meta_size = encoded_key.size() + value.size();
    hash_info[encoded_key.ToString()] = std::make_tuple(meta_size, ttl, parsed_meta.Version());
  }
  
  // Second pass: scan data and accumulate sizes
  std::unique_ptr<rocksdb::Iterator> data_iter(db->NewIterator(read_options, data_handle));
  
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    const rocksdb::Slice& data_key = data_iter->key();
    const rocksdb::Slice& data_value = data_iter->value();
    
    // Extract the encoded user key from data key
    // Data key format: encoded_key + version + field
    const char* ptr = storage::SeekUserkeyDelim(data_key.data(), data_key.size());
    size_t user_key_len = ptr - data_key.data();
    
    if (user_key_len == 0 || user_key_len > data_key.size()) continue;
    
    std::string encoded_user_key(data_key.data(), user_key_len);
    
    auto it = hash_info.find(encoded_user_key);
    if (it != hash_info.end()) {
      std::get<0>(it->second) += data_key.size() + data_value.size();
    }
  }
  
  // Add results
  for (const auto& entry : hash_info) {
    int64_t size = std::get<0>(entry.second);
    if (size >= config.min_size) {
      std::string user_key = DecodeUserKey(entry.first);
      std::string display_key = ReplaceAll(user_key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("hash", std::move(display_key), size, std::get<1>(entry.second));
    }
  }
}

// Analyze sets
void AnalyzeSets(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* meta_handle,
                rocksdb::ColumnFamilyHandle* data_handle,
                std::vector<KeyInfo>& key_infos, const Config& config) {
  std::cout << "Analyzing sets..." << std::endl;
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  curtime *= 1000;
  
  rocksdb::ReadOptions read_options;
  std::unique_ptr<rocksdb::Iterator> meta_iter(db->NewIterator(read_options, meta_handle));
  
  std::unordered_map<std::string, std::tuple<int64_t, int64_t, uint64_t>> set_info;
  
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    const rocksdb::Slice& encoded_key = meta_iter->key();
    const rocksdb::Slice& value = meta_iter->value();
    
    if (value.size() < 1) continue;
    
    storage::DataType type = static_cast<storage::DataType>(static_cast<uint8_t>(value[0]));
    if (type != storage::DataType::kSets) {
      continue;
    }
    
    std::string value_str = value.ToString();
    storage::ParsedSetsMetaValue parsed_meta(&value_str);
    
    if (parsed_meta.IsStale() || parsed_meta.Count() == 0) {
      continue;
    }
    
    int64_t ttl = -1;
    if (!parsed_meta.IsPermanentSurvival()) {
      int64_t etime = parsed_meta.Etime();
      if (etime > curtime) {
        ttl = (etime - curtime) / 1000;
      }
    }
    
    int64_t meta_size = encoded_key.size() + value.size();
    set_info[encoded_key.ToString()] = std::make_tuple(meta_size, ttl, parsed_meta.Version());
  }
  
  std::unique_ptr<rocksdb::Iterator> data_iter(db->NewIterator(read_options, data_handle));
  
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    const rocksdb::Slice& data_key = data_iter->key();
    const rocksdb::Slice& data_value = data_iter->value();
    
    const char* ptr = storage::SeekUserkeyDelim(data_key.data(), data_key.size());
    size_t user_key_len = ptr - data_key.data();
    
    if (user_key_len == 0 || user_key_len > data_key.size()) continue;
    
    std::string encoded_user_key(data_key.data(), user_key_len);
    
    auto it = set_info.find(encoded_user_key);
    if (it != set_info.end()) {
      std::get<0>(it->second) += data_key.size() + data_value.size();
    }
  }
  
  for (const auto& entry : set_info) {
    int64_t size = std::get<0>(entry.second);
    if (size >= config.min_size) {
      std::string user_key = DecodeUserKey(entry.first);
      std::string display_key = ReplaceAll(user_key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("set", std::move(display_key), size, std::get<1>(entry.second));
    }
  }
}

// Analyze zsets
void AnalyzeZsets(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* meta_handle,
                 rocksdb::ColumnFamilyHandle* data_handle,
                 rocksdb::ColumnFamilyHandle* score_handle,
                 std::vector<KeyInfo>& key_infos, const Config& config) {
  std::cout << "Analyzing zsets..." << std::endl;
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  curtime *= 1000;
  
  rocksdb::ReadOptions read_options;
  std::unique_ptr<rocksdb::Iterator> meta_iter(db->NewIterator(read_options, meta_handle));
  
  std::unordered_map<std::string, std::tuple<int64_t, int64_t, uint64_t>> zset_info;
  
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    const rocksdb::Slice& encoded_key = meta_iter->key();
    const rocksdb::Slice& value = meta_iter->value();
    
    if (value.size() < 1) continue;
    
    storage::DataType type = static_cast<storage::DataType>(static_cast<uint8_t>(value[0]));
    if (type != storage::DataType::kZSets) {
      continue;
    }
    
    std::string value_str = value.ToString();
    storage::ParsedZSetsMetaValue parsed_meta(&value_str);
    
    if (parsed_meta.IsStale() || parsed_meta.Count() == 0) {
      continue;
    }
    
    int64_t ttl = -1;
    if (!parsed_meta.IsPermanentSurvival()) {
      int64_t etime = parsed_meta.Etime();
      if (etime > curtime) {
        ttl = (etime - curtime) / 1000;
      }
    }
    
    int64_t meta_size = encoded_key.size() + value.size();
    zset_info[encoded_key.ToString()] = std::make_tuple(meta_size, ttl, parsed_meta.Version());
  }
  
  // Scan data CF
  std::unique_ptr<rocksdb::Iterator> data_iter(db->NewIterator(read_options, data_handle));
  
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    const rocksdb::Slice& data_key = data_iter->key();
    const rocksdb::Slice& data_value = data_iter->value();
    
    const char* ptr = storage::SeekUserkeyDelim(data_key.data(), data_key.size());
    size_t user_key_len = ptr - data_key.data();
    
    if (user_key_len == 0 || user_key_len > data_key.size()) continue;
    
    std::string encoded_user_key(data_key.data(), user_key_len);
    
    auto it = zset_info.find(encoded_user_key);
    if (it != zset_info.end()) {
      std::get<0>(it->second) += data_key.size() + data_value.size();
    }
  }
  
  // Scan score CF
  std::unique_ptr<rocksdb::Iterator> score_iter(db->NewIterator(read_options, score_handle));
  
  for (score_iter->SeekToFirst(); score_iter->Valid(); score_iter->Next()) {
    const rocksdb::Slice& score_key = score_iter->key();
    const rocksdb::Slice& score_value = score_iter->value();
    
    // Parse the score key using ParsedZSetsScoreKey
    try {
      storage::ParsedZSetsScoreKey parsed_key(score_key);
      std::string encoded_user_key = parsed_key.key().ToString();
      
      auto it = zset_info.find(encoded_user_key);
      if (it != zset_info.end()) {
        std::get<0>(it->second) += score_key.size() + score_value.size();
      }
    } catch (...) {
      // Skip malformed keys
      continue;
    }
  }
  
  for (const auto& entry : zset_info) {
    int64_t size = std::get<0>(entry.second);
    if (size >= config.min_size) {
      std::string user_key = DecodeUserKey(entry.first);
      std::string display_key = ReplaceAll(user_key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("zset", std::move(display_key), size, std::get<1>(entry.second));
    }
  }
}

// Analyze lists
void AnalyzeLists(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* meta_handle,
                 rocksdb::ColumnFamilyHandle* data_handle,
                 std::vector<KeyInfo>& key_infos, const Config& config) {
  std::cout << "Analyzing lists..." << std::endl;
  
  int64_t curtime;
  db->GetEnv()->GetCurrentTime(&curtime).ok();
  curtime *= 1000;
  
  rocksdb::ReadOptions read_options;
  std::unique_ptr<rocksdb::Iterator> meta_iter(db->NewIterator(read_options, meta_handle));
  
  std::unordered_map<std::string, std::tuple<int64_t, int64_t, uint64_t>> list_info;
  
  for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
    const rocksdb::Slice& encoded_key = meta_iter->key();
    const rocksdb::Slice& value = meta_iter->value();
    
    if (value.size() < 1) continue;
    
    storage::DataType type = static_cast<storage::DataType>(static_cast<uint8_t>(value[0]));
    if (type != storage::DataType::kLists) {
      continue;
    }
    
    std::string value_str = value.ToString();
    storage::ParsedListsMetaValue parsed_meta(&value_str);
    
    if (parsed_meta.IsStale() || parsed_meta.Count() == 0) {
      continue;
    }
    
    int64_t ttl = -1;
    if (!parsed_meta.IsPermanentSurvival()) {
      int64_t etime = parsed_meta.Etime();
      if (etime > curtime) {
        ttl = (etime - curtime) / 1000;
      }
    }
    
    int64_t meta_size = encoded_key.size() + value.size();
    list_info[encoded_key.ToString()] = std::make_tuple(meta_size, ttl, parsed_meta.Version());
  }
  
  std::unique_ptr<rocksdb::Iterator> data_iter(db->NewIterator(read_options, data_handle));
  
  for (data_iter->SeekToFirst(); data_iter->Valid(); data_iter->Next()) {
    const rocksdb::Slice& data_key = data_iter->key();
    const rocksdb::Slice& data_value = data_iter->value();
    
    // Parse the data key using ParsedListsDataKey
    try {
      storage::ParsedListsDataKey parsed_key(data_key);
      std::string encoded_user_key = parsed_key.key().ToString();
      
      auto it = list_info.find(encoded_user_key);
      if (it != list_info.end()) {
        std::get<0>(it->second) += data_key.size() + data_value.size();
      }
    } catch (...) {
      // Skip malformed keys
      continue;
    }
  }
  
  for (const auto& entry : list_info) {
    int64_t size = std::get<0>(entry.second);
    if (size >= config.min_size) {
      std::string user_key = DecodeUserKey(entry.first);
      std::string display_key = ReplaceAll(user_key, "\n", "\\n");
      display_key = ReplaceAll(display_key, " ", "\\x20");
      key_infos.emplace_back("list", std::move(display_key), size, std::get<1>(entry.second));
    }
  }
}

// Get the prefix of a key
std::string GetKeyPrefix(const std::string& key, const std::string& delimiter) {
  size_t pos = key.find(delimiter);
  if (pos != std::string::npos) {
    return key.substr(0, pos);
  }
  return key;
}

// Generate prefix statistics
void GeneratePrefixStats(const std::vector<KeyInfo>& key_infos, const std::string& delimiter, std::ostream& out) {
  std::unordered_map<std::string, PrefixStat> prefix_stats;
  
  for (const auto& info : key_infos) {
    std::string prefix = GetKeyPrefix(info.key, delimiter);
    prefix_stats[prefix].Add(info.size);
  }
  
  std::vector<std::pair<std::string, PrefixStat>> sorted_stats;
  for (const auto& entry : prefix_stats) {
    sorted_stats.emplace_back(entry);
  }
  
  std::sort(sorted_stats.begin(), sorted_stats.end(), 
    [](const auto& a, const auto& b) {
      return a.second.total_size > b.second.total_size;
    });
  
  out << "\n===== Key Prefix Statistics =====\n";
  out << "Prefix\tCount\tTotal Size\tAvg Size\n";
  
  for (const auto& entry : sorted_stats) {
    double avg_size = static_cast<double>(entry.second.total_size) / entry.second.count;
    out << entry.first << "\t" 
        << entry.second.count << "\t" 
        << entry.second.total_size << "\t"
        << avg_size << "\n";
  }
}

// Analyze a single database instance
void AnalyzeSingleDB(const std::string& db_path, std::vector<KeyInfo>& key_infos, const Config& config) {
  rocksdb::DBOptions db_options;
  db_options.create_if_missing = false;
  
  std::vector<std::string> cf_names = {
    "default",         // kMetaCF
    "hashes_data_cf",  // kHashesDataCF
    "sets_data_cf",    // kSetsDataCF
    "lists_data_cf",   // kListsDataCF
    "zsets_data_cf",   // kZsetsDataCF
    "zsets_score_cf",  // kZsetsScoreCF
    "streams_data_cf"  // kStreamsDataCF
  };
  
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  for (const auto& cf_name : cf_names) {
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        cf_name, rocksdb::ColumnFamilyOptions()));
  }
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(db_options, db_path, 
                                                         column_families, &handles, &db);
  
  if (!status.ok()) {
    std::cerr << "Error opening database at " << db_path << ": " << status.ToString() << std::endl;
    return;
  }
  
  std::cout << "Analyzing database at " << db_path << std::endl;
  
  // Analyze each type
  if (config.type_filter == "all" || config.type_filter == "strings") {
    AnalyzeStrings(db, handles[storage::kMetaCF], key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "hashes") {
    AnalyzeHashes(db, handles[storage::kMetaCF], handles[storage::kHashesDataCF], key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "sets") {
    AnalyzeSets(db, handles[storage::kMetaCF], handles[storage::kSetsDataCF], key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "zsets") {
    AnalyzeZsets(db, handles[storage::kMetaCF], handles[storage::kZsetsDataCF], 
                 handles[storage::kZsetsScoreCF], key_infos, config);
  }
  
  if (config.type_filter == "all" || config.type_filter == "lists") {
    AnalyzeLists(db, handles[storage::kMetaCF], handles[storage::kListsDataCF], key_infos, config);
  }
  
  // Cleanup
  for (auto handle : handles) {
    delete handle;
  }
  delete db;
}

int main(int argc, char *argv[]){
  Config config;
  if (!ParseArgs(argc, argv, config)) {
    return 1;
  }
  
  std::vector<KeyInfo> key_infos;
  
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
  
  // Check if this is a single DB or multiple DB instances
  // Try to detect db/0, db/1, db/2, etc.
  std::vector<std::string> db_paths;
  
  // First, check if db_path itself is a valid RocksDB
  std::string test_path = config.db_path;
  if (DirectoryExists(test_path + "/CURRENT")) {
    // This is a single database instance
    db_paths.push_back(test_path);
    std::cout << "Detected single database instance" << std::endl;
  } else {
    // Check for multiple database instances (db/0, db/1, db/2, ...)
    int db_index = 0;
    while (true) {
      std::string db_inst_path = config.db_path + "/db/" + std::to_string(db_index);
      if (DirectoryExists(db_inst_path) && DirectoryExists(db_inst_path + "/CURRENT")) {
        db_paths.push_back(db_inst_path);
        db_index++;
      } else {
        break;
      }
    }
    
    if (db_paths.empty()) {
      std::cerr << "Error: No valid database found at " << config.db_path << std::endl;
      std::cerr << "Checked for single instance and db/0, db/1, ... directories" << std::endl;
      return 1;
    }
    
    std::cout << "Detected " << db_paths.size() << " database instances" << std::endl;
  }
  
  // Analyze each database instance
  for (const auto& db_path : db_paths) {
    AnalyzeSingleDB(db_path, key_infos, config);
  }
  
  // Sort keys by size
  std::sort(key_infos.begin(), key_infos.end());
  
  // Limit to top N if requested
  if (config.top_n > 0 && config.top_n < static_cast<int>(key_infos.size())) {
    key_infos.resize(config.top_n);
  }
  
  // Output results
  *out << "===== Big Key Analysis =====\n";
  *out << "Type\tSize\tKey\tTTL\n";
  
  for (const auto& info : key_infos) {
    *out << info.type << "\t" << info.size << "\t" << info.key << "\t" << info.ttl << "\n";
  }
  
  // Generate prefix statistics if requested
  if (config.prefix_stat) {
    GeneratePrefixStats(key_infos, config.prefix_delimiter, *out);
  }
  
  // Output summary
  *out << "\n===== Summary =====\n";
  *out << "Total keys analyzed: " << key_infos.size() << "\n";
  
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
