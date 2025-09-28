#pragma once
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/transfer/TransferManager.h>
#include <memory>
#include <mutex>
#include <string>
#include "sst_downloader.h"

class S3Service {
 public:
  S3Service();
  ~S3Service();

  bool Start(const std::string& conf_path, std::string* err);
  void Stop();

  std::shared_ptr<Aws::S3::S3Client> Client();
  std::shared_ptr<Aws::Transfer::TransferManager> TransferMgr();

  const std::string& Bucket() const;
  int TransferThreads() const;
  size_t TransferBufBytes() const;
  size_t MaxInflight() const;

  int RetryMaxAttempts() const;
  int RetryBaseMs() const;
  int RetryMaxMs() const;
  double RetryJitter() const;

  SstDownloader* Downloader() { return downloader_.get(); }

 private:
  // 配置缓存
  std::string conf_region_;
  std::string conf_endpoint_;
  std::string conf_ak_;
  std::string conf_sk_;

  // 运行期参数
  std::string bucket_;
  int transfer_threads_ = 8;
  size_t transfer_buf_bytes_ = 8u << 20;
  size_t max_inflight_ = 8;

  // AWS
  Aws::SDKOptions options_;
  std::shared_ptr<Aws::S3::S3Client> client_;
  std::shared_ptr<Aws::Transfer::TransferManager> xfer_mgr_;
  std::shared_ptr<Aws::Utils::Threading::PooledThreadExecutor> xfer_pool_;

  // dowmload sst
  int retry_max_attempts_ = 3;  
  int retry_base_ms_ = 50;     
  int retry_max_ms_ = 2000;     
  double retry_jitter_ = 0.2;  
  std::unique_ptr<SstDownloader> downloader_;

  // 状态
  bool started_ = false;
  mutable std::mutex mu_;
};
