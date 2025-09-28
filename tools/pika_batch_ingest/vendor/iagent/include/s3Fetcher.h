#pragma once
#include "configLoader.h"
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace iagent {

struct LastManifest {
  std::vector<std::string> parts; // latest.manifest 中列出的 .manifest 列表
  static LastManifest from_json(const nlohmann::json &j) {
    LastManifest lm;
    if (j.contains("parts") && j["parts"].is_array()) {
      for (auto &v : j["parts"]) {
        if (v.is_string())
          lm.parts.emplace_back(v.get<std::string>());
      }
    }
    return lm;
  }
};

class S3Fetcher {
public:
  using ManifestCallback = std::function<void(const std::string &manifestKey)>;

  explicit S3Fetcher(const S3Config &config);
  ~S3Fetcher();

  void start(const ManifestCallback &onManifestUpdate);
  void stop();
  bool fetchLast(std::string &contentOut);
  bool fetchObject(const std::string &key, std::string &contentOut);

private:
  void pollingLoop(const ManifestCallback &callback);
  std::string computeMD5(const std::string &data);
  LastManifest extractLastManifestFile(const std::string &content);

private:
  S3Config config_;
  std::string lastSeenHash_;
};

} // namespace iagent
