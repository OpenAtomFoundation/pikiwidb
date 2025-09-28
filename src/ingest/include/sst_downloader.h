#ifndef PIKA_SST_DOWNLOADER_H_
#define PIKA_SST_DOWNLOADER_H_

#include <aws/s3/S3Client.h>
#include <aws/transfer/TransferManager.h>
#include <rocksdb/status.h>
#include <memory>
#include <string>
#include <vector>

// 负责下载 manifest 和 sst 文件的类
class SstDownloader {
 public:
  SstDownloader(std::shared_ptr<Aws::S3::S3Client> client,
                std::shared_ptr<Aws::Transfer::TransferManager> xfer_mgr,
                const std::string& bucket,
                const std::string& manifest_dict,
                const std::string& sst_dict);

  ~SstDownloader();

  SstDownloader(const SstDownloader&) = delete;
  SstDownloader& operator=(const SstDownloader&) = delete;
  SstDownloader(SstDownloader&&) = default;
  SstDownloader& operator=(SstDownloader&&) = default;

  // 下载并返回 SST 文件路径
  rocksdb::Status DownloadAllFiles(const std::string& manifest_name,
                                   std::vector<std::string>& out_sst_files);

 private:
  rocksdb::Status ParseManifest(const std::string& manifest_key,
                                std::vector<std::string>& out_keys);

  std::shared_ptr<Aws::S3::S3Client> client_;
  std::shared_ptr<Aws::Transfer::TransferManager> xfer_mgr_;
  std::string bucket_;
  std::string manifest_dict_;
  std::string sst_dict_;
};

#endif  // PIKA_SST_DOWNLOADER_H_
