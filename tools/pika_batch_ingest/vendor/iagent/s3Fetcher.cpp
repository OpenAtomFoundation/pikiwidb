#include "s3Fetcher.h"
#include "include/configLoader.h"
#include "utils/klog.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/http/HttpTypes.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <iomanip>
#include <openssl/evp.h>
#include <regex>
#include <sstream>

namespace iagent {

S3Fetcher::S3Fetcher(const S3Config &config) : config_(config) {}
S3Fetcher::~S3Fetcher() { stop(); }

void S3Fetcher::start(const ManifestCallback &onManifestUpdate) {
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  pollingLoop(onManifestUpdate);
  Aws::ShutdownAPI(options);
}

void S3Fetcher::stop() {
  // 若做常驻轮询，这里设置 running 标志退出
}

static void parseEndpointAndScheme(const std::string &endpoint,
                                   std::string &outEndpoint,
                                   Aws::Http::Scheme &outScheme) {
  outEndpoint.clear();
  if (endpoint.rfind("http://", 0) == 0) {
    outScheme = Aws::Http::Scheme::HTTP;
    outEndpoint = endpoint.substr(7);

  } else if (endpoint.rfind("https://", 0) == 0) {
    outScheme = Aws::Http::Scheme::HTTPS;
    outEndpoint = endpoint.substr(8); 

  } else {
    outScheme = Aws::Http::Scheme::HTTPS;
    outEndpoint = endpoint;
  }
}

static Aws::S3::S3Client makeS3Client(const S3Config &cfg) {
  Aws::Client::ClientConfiguration clientConfig;

  std::string endpointStripped;
  Aws::Http::Scheme scheme;

  if (!cfg.endpoint.empty()) {
    parseEndpointAndScheme(cfg.endpoint, endpointStripped, scheme);
    clientConfig.endpointOverride = endpointStripped;
    clientConfig.region = cfg.region.empty() ? "us-east-1" : cfg.region;
  } else {
    scheme = Aws::Http::Scheme::HTTPS;
    clientConfig.region = cfg.region;
  }
  clientConfig.scheme = scheme;

  clientConfig.connectTimeoutMs = cfg.connect_timeout_ms;
  clientConfig.requestTimeoutMs = cfg.rw_timeout_ms;

  Aws::Auth::AWSCredentials credentials(cfg.accessKey, cfg.secretKey);

  return Aws::S3::S3Client(
      credentials, clientConfig,
      Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::RequestDependent,
      /*useVirtualAddressing*/ true);
}

void S3Fetcher::pollingLoop(const ManifestCallback &callback) {
  std::string content;
  if (fetchLast(content)) {
    std::string currentHash = computeMD5(content);
    if (currentHash != lastSeenHash_) {
      lastSeenHash_ = currentHash;
      try {
        LastManifest lm = extractLastManifestFile(content);
        for (auto &manifestKey : lm.parts) {
          if (!manifestKey.empty()) {
            LOG_DEBUG("[S3Fetcher] Parsed manifest_file: " + manifestKey);
            callback(manifestKey);
          } else {
            LOG_ERROR(
                "[S3Fetcher] Failed to extract manifest_file from content.");
          }
        }
      } catch (const std::exception& e) {
        LOG_ERROR(
            std::string("[S3Fetcher] Failed to parse latest.manifest: ") + e.what());
      }
    }
  } else {
    LOG_ERROR("[S3Fetcher] Failed to fetch latest.manifest.");
  }
}

LastManifest S3Fetcher::extractLastManifestFile(const std::string &content) {
  try {
    nlohmann::json j = nlohmann::json::parse(content);
    return LastManifest::from_json(j);
  } catch (const std::exception& e) {
    LOG_ERROR(std::string("[S3Fetcher] Failed to parse JSON: ") + e.what());
    throw; 
  }
}

bool S3Fetcher::fetchLast(std::string &contentOut) {
  auto s3_client = makeS3Client(config_);

  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(config_.bucket);
  const std::string key = (!config_.key.empty() && config_.key.front() == '/')
                              ? config_.key.substr(1)
                              : config_.key;
  request.SetKey(key);

  auto outcome = s3_client.GetObject(request);
  if (!outcome.IsSuccess()) {
    const auto &err = outcome.GetError();
    LOG_ERROR(
        std::string("[S3Fetcher] Failed to fetch latest.manifest. Exception=") +
        err.GetExceptionName() + " Message=" + err.GetMessage());
    return false;
  }

  std::stringstream ss;
  ss << outcome.GetResult().GetBody().rdbuf();
  contentOut = ss.str();
  return true;
}

bool S3Fetcher::fetchObject(const std::string &key, std::string &contentOut) {
  auto s3_client = makeS3Client(config_);

  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(config_.bucket);
  const std::string normKey =
      (!key.empty() && key.front() == '/') ? key.substr(1) : key;
  request.SetKey(normKey);

  auto outcome = s3_client.GetObject(request);
  if (!outcome.IsSuccess()) {
    const auto &err = outcome.GetError();
    LOG_ERROR(std::string("[S3Fetcher] fetchObject failed: ") + normKey +
              " Exception=" + err.GetExceptionName() +
              " Message=" + err.GetMessage());
    return false;
  }

  std::stringstream ss;
  ss << outcome.GetResult().GetBody().rdbuf();
  contentOut = ss.str();
  return true;
}

std::string S3Fetcher::computeMD5(const std::string &data) {
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len = 0;

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(mdctx, EVP_md5(), nullptr);
  EVP_DigestUpdate(mdctx, data.c_str(), data.size());
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);

  std::ostringstream oss;
  for (unsigned int i = 0; i < md_len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(md_value[i]);
  }
  return oss.str();
}

} // namespace iagent
