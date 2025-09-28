#ifndef IAGENT_CONFIG_LOADER_H
#define IAGENT_CONFIG_LOADER_H

#include <string>
namespace iagent {

struct S3Config {
  std::string endpoint;
  std::string region;
  std::string bucket;
  std::string key;
  std::string accessKey;
  std::string secretKey;
  int manifest_batch;
  int connect_timeout_ms;
  int rw_timeout_ms;
  int max_retries;
};

struct PikaConfig {
  std::string host;
  int port;
};

class ConfigLoader {
public:
  static S3Config loadS3Config(const std::string &filePath);
  static PikaConfig loadPikaConfig(const std::string &filePath);
};

} // namespace iagent

#endif // IAGENT_CONFIG_LOADER_H