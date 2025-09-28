#include "ingest_s3_service.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>
#include <aws/s3/S3Client.h>
#include <aws/transfer/TransferManager.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>

#include "nlohmann/json.hpp"
#include "pstd/include/pstd_string.h"

using json = nlohmann::json;

namespace {

bool ReadAll(const std::string& path, std::string* out) {
  std::ifstream in(path);
  if (!in.is_open()) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

template <typename T>
bool Require(const json& j, const char* key, T* out) {
  if (!j.contains(key)) return false;
  try {
    *out = j.at(key).get<T>();
    return true;
  } catch (...) {
    return false;
  }
}

template <typename T>
void Optional(const json& j, const char* key, T* out) {
  try {
    if (j.contains(key)) *out = j.at(key).get<T>();
  } catch (...) {
  }
}

}  // namespace

S3Service::S3Service() = default;
S3Service::~S3Service() { Stop(); }

bool S3Service::Start(const std::string& conf_path, std::string* err) {
  std::lock_guard<std::mutex> lk(mu_);
  if (started_) return true;

  // Init AWS SDK
  try {
    Aws::InitAPI(options_);
  } catch (...) {
    if (err) *err = "Aws::InitAPI failed";
    return false;
  }

  // 读取配置
  std::string text;
  if (!ReadAll(conf_path, &text)) {
    if (err) *err = "Open config failed: " + conf_path;
    Aws::ShutdownAPI(options_);
    return false;
  }

  json j;
  try {
    j = json::parse(text, /*cb=*/nullptr, /*allow_exceptions=*/true, /*ignore_comments=*/true);
  } catch (const std::exception& e) {
    if (err) *err = std::string("Parse json failed: ") + e.what();
    Aws::ShutdownAPI(options_);
    return false;
  }

  // 填充配置
  conf_region_ = "us-east-1";
  Optional(j, "region", &conf_region_);
  Optional(j, "endpoint", &conf_endpoint_);
  if (!Require(j, "bucket", &bucket_)) {
    if (err) *err = "Missing field: bucket";
    Aws::ShutdownAPI(options_);
    return false;
  }
  Optional(j, "access_key", &conf_ak_);
  Optional(j, "secret_key", &conf_sk_);
  Optional(j, "transfer_threads", &transfer_threads_);
  Optional(j, "transfer_buf_bytes", &transfer_buf_bytes_);
  Optional(j, "max_inflight", &max_inflight_);

  if (transfer_threads_ <= 0) transfer_threads_ = 8;
  if (transfer_buf_bytes_ == 0) transfer_buf_bytes_ = (8u << 20);
  if (max_inflight_ == 0) max_inflight_ = 8;

  // 构建 ClientConfiguration
  Aws::Client::ClientConfiguration cfg;
  cfg.region = conf_region_.empty() ? "us-east-1" : conf_region_;
  if (!conf_region_.empty()) {
    cfg.endpointOverride = conf_endpoint_;
    if (conf_endpoint_.rfind("http://", 0) == 0)
      cfg.scheme = Aws::Http::Scheme::HTTP;
    else
      cfg.scheme = Aws::Http::Scheme::HTTPS;
  }

  // 创建 S3Client
  try {
    if (!conf_ak_.empty() && !conf_sk_.empty()) {
      Aws::Auth::AWSCredentials creds(conf_ak_.c_str(), conf_sk_.c_str());
      client_ = std::make_shared<Aws::S3::S3Client>(
          creds, cfg, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::RequestDependent,
          /*useVirtualAddressing=*/true, Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::LEGACY);
    } else {
      client_ = std::make_shared<Aws::S3::S3Client>(
          cfg, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::RequestDependent,
          /*useVirtualAddressing=*/true, Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::LEGACY);
    }
  } catch (const std::exception& e) {
    if (err) *err = std::string("Create S3Client failed: ") + e.what();
    client_.reset();
    Aws::ShutdownAPI(options_);
    return false;
  } catch (...) {
    if (err) *err = "Create S3Client failed (unknown)";
    client_.reset();
    Aws::ShutdownAPI(options_);
    return false;
  }

  // 读取配置
  Optional(j, "retry_max_attempts", &retry_max_attempts_);
  Optional(j, "retry_base_ms", &retry_base_ms_);
  Optional(j, "retry_max_ms", &retry_max_ms_);
  Optional(j, "retry_jitter", &retry_jitter_);

  // 创建 TransferManager
  try {
    xfer_pool_ =
        Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("PikaS3XferPool", std::max(1, transfer_threads_));

    Aws::Transfer::TransferManagerConfiguration tcfg(xfer_pool_.get());
    tcfg.s3Client = client_;
    tcfg.transferExecutor = xfer_pool_.get();

    tcfg.bufferSize = transfer_buf_bytes_;
    const size_t min_total = transfer_buf_bytes_;
    const size_t heuristic = transfer_buf_bytes_ * static_cast<size_t>(std::max(1, transfer_threads_)) * 2;
    tcfg.transferBufferMaxHeapSize = std::max(min_total, heuristic);

    tcfg.transferInitiatedCallback = [](const Aws::Transfer::TransferManager*,
                                        const std::shared_ptr<const Aws::Transfer::TransferHandle>&) {};

    xfer_mgr_ = Aws::Transfer::TransferManager::Create(tcfg);
  } catch (const std::exception& e) {
    if (err) *err = std::string("Create TransferManager failed: ") + e.what();
    xfer_mgr_.reset();
    xfer_pool_.reset();
    client_.reset();
    Aws::ShutdownAPI(options_);
    return false;
  } catch (...) {
    if (err) *err = "Create TransferManager failed (unknown)";
    xfer_mgr_.reset();
    xfer_pool_.reset();
    client_.reset();
    Aws::ShutdownAPI(options_);
    return false;
  }

  // 初始化sst downloader
  downloader_ = std::make_unique<SstDownloader>(client_, xfer_mgr_, bucket_, "manifest/", "sst/");

  started_ = true;
  return true;
}

void S3Service::Stop() {
  std::lock_guard<std::mutex> lk(mu_);
  if (!started_) return;

  xfer_mgr_.reset();
  xfer_pool_.reset();
  client_.reset();

  try {
    Aws::ShutdownAPI(options_);
  } catch (...) {
  }
  started_ = false;
}

std::shared_ptr<Aws::S3::S3Client> S3Service::Client() {
  std::lock_guard<std::mutex> lk(mu_);
  return client_;
}

std::shared_ptr<Aws::Transfer::TransferManager> S3Service::TransferMgr() {
  std::lock_guard<std::mutex> lk(mu_);
  return xfer_mgr_;
}

const std::string& S3Service::Bucket() const { return bucket_; }

int S3Service::TransferThreads() const { return transfer_threads_; }

size_t S3Service::TransferBufBytes() const { return transfer_buf_bytes_; }

size_t S3Service::MaxInflight() const { return max_inflight_; }

int S3Service::RetryMaxAttempts() const { return retry_max_attempts_; }

int S3Service::RetryBaseMs() const { return retry_base_ms_; }

int S3Service::RetryMaxMs() const { return retry_max_ms_; }

double S3Service::RetryJitter() const { return retry_jitter_; }