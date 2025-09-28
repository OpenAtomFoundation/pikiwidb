#include "configLoader.h"
#include "utils/klog.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace iagent {

S3Config ConfigLoader::loadS3Config(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file) {
    LOG_ERROR("Failed to open S3 config file: " + filePath);
    throw std::runtime_error("Failed to open S3 config file: " + filePath);
  }

  json j;
  file >> j;
  
  if (!j.contains("endpoint") || !j.contains("region") || !j.contains("bucket") || 
      !j.contains("key") || !j.contains("access_key") || !j.contains("secret_key") ||
      !j.contains("manifest_batch") || !j.contains("connect_timeout_ms") || 
      !j.contains("rw_timeout_ms")) {
    LOG_ERROR("Missing required fields in S3 config file: " + filePath);
    throw std::runtime_error("Missing required fields in S3 config file: " + filePath);
  }

  return S3Config{j.at("endpoint").get<std::string>(),
                  j.at("region").get<std::string>(),
                  j.at("bucket").get<std::string>(),
                  j.at("key").get<std::string>(),
                  j.at("access_key").get<std::string>(),
                  j.at("secret_key").get<std::string>(),
                  j.at("manifest_batch").get<int>(),
                  j.at("connect_timeout_ms").get<int>(),
                  j.at("rw_timeout_ms").get<int>(),
                  j.at("max_retries").get<int>()};
}

PikaConfig ConfigLoader::loadPikaConfig(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file) {
    LOG_ERROR("Failed to open Pika config file: " + filePath);
    throw std::runtime_error("Failed to open Pika config file: " + filePath);
  }

  json j;
  file >> j;
  
  if (!j.contains("host") || !j.contains("port")) {
    LOG_ERROR("Missing required fields in Pika config file: " + filePath);
    throw std::runtime_error("Missing required fields in Pika config file: " + filePath);
  }

  return PikaConfig{j.at("host").get<std::string>(), j.at("port").get<int>()};
}

} // namespace iagent
