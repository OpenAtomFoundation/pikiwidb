#include "sst_downloader.h"
#include <aws/core/Aws.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <glog/logging.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "manifest.pb.h"

SstDownloader::SstDownloader(std::shared_ptr<Aws::S3::S3Client> client,
                             std::shared_ptr<Aws::Transfer::TransferManager> xfer_mgr,
                             const std::string& bucket,
                             const std::string& manifest_dict,
                             const std::string& sst_dict)
    : client_(std::move(client)),
      xfer_mgr_(std::move(xfer_mgr)),
      bucket_(bucket),
      manifest_dict_(manifest_dict),
      sst_dict_(sst_dict) {
      LOG(INFO) << "[SstDownloader] Initialized with"
                << " bucket=" << bucket_
                << " manifest_dict=" << manifest_dict_
                << " sst_dict=" << sst_dict_;
}

SstDownloader::~SstDownloader() {
  LOG(INFO) << "[SstDownloader] Destroyed, releasing S3 client and transfer manager";
  client_.reset();
  xfer_mgr_.reset();
}

rocksdb::Status SstDownloader::ParseManifest(const std::string& manifest_key,
                                             std::vector<std::string>& out_keys) {
  // LOG(INFO) << "[SstDownloader] Downloading manifest object from S3:"
  //           << " bucket=" << bucket_
  //           << " key=" << manifest_key;

  Aws::S3::Model::GetObjectRequest req;
  req.SetBucket(bucket_);
  req.SetKey(manifest_key);

  auto outcome = client_->GetObject(req);
  if (!outcome.IsSuccess()) {
    auto err = outcome.GetError();
    LOG(ERROR) << "[SstDownloader] Failed to download manifest"
               << " bucket=" << bucket_
               << " key=" << manifest_key
               << " error=" << err.GetMessage()
               << " http=" << static_cast<int>(err.GetResponseCode())
               << " s3err=" << static_cast<int>(err.GetErrorType());
    return rocksdb::Status::IOError("Failed to download manifest: " + err.GetMessage());
  }

  std::stringstream ss;
  ss << outcome.GetResult().GetBody().rdbuf();
  ManifestIngest::Manifest manifest;
  if (!manifest.ParseFromString(ss.str())) {
    LOG(ERROR) << "[SstDownloader] Failed to parse manifest content (protobuf parse failed)";
    return rocksdb::Status::Corruption("Failed to parse manifest content");
  }

  // LOG(INFO) << "[SstDownloader] Manifest parsed successfully"
  //           << " version=" << manifest.version_id()
  //           << " ts=" << manifest.timestamp()
  //           << " files=" << manifest.sst_files_size();

  out_keys.clear();
  out_keys.reserve(manifest.sst_files_size());
  for (const auto& s : manifest.sst_files()) {
    // LOG(INFO) << "[SstDownloader] Found SST key in manifest: " << s.sst_path();
    out_keys.push_back(s.sst_path());
  }

  return rocksdb::Status::OK();
}

rocksdb::Status SstDownloader::DownloadAllFiles(const std::string& manifest_name,
                                                std::vector<std::string>& out_sst_files) {
  const std::string manifest_key = manifest_dict_ + manifest_name;
  std::vector<std::string> keys;

  auto st = ParseManifest(manifest_key, keys);
  if (!st.ok()) return st;

  if (keys.empty()) {
    LOG(WARNING) << "[SstDownloader] Manifest contains no SST files";
    return rocksdb::Status::OK();
  }

  out_sst_files.clear();
  out_sst_files.reserve(keys.size());

  namespace fs = std::filesystem;

  for (const auto& key : keys) {
    std::string normalized_key = key;
    while (!normalized_key.empty() &&
           (normalized_key.front() == '/' || normalized_key.front() == '\\')) {
      normalized_key.erase(normalized_key.begin());
    }

    if (normalized_key.empty() || normalized_key == "." ||
        normalized_key == ".." || normalized_key.find("..") != std::string::npos) {
      return rocksdb::Status::Corruption("Invalid sst key: " + normalized_key);
    }
    
    std::string s3_key = normalized_key;
    if (s3_key.rfind("sst/", 0) != 0) {
      s3_key = sst_dict_ + s3_key;
    }

    fs::path local_path = fs::path("data/") / normalized_key;
    std::error_code ec;
    fs::create_directories(local_path.parent_path(), ec);

    Aws::Transfer::DownloadConfiguration dcfg;
    auto handle = xfer_mgr_->DownloadFile(
        bucket_.c_str(),
        Aws::String(s3_key.c_str()), 
        [local_path]() {
          return Aws::New<Aws::FStream>(
              "S3DownloadStream",
              local_path.string().c_str(),
              std::ios::out | std::ios::binary | std::ios::trunc);
        },
        dcfg,
        Aws::String(local_path.string().c_str()));

    if (!handle) {
      return rocksdb::Status::IOError("Null transfer handle for key: " + s3_key);
    }

    handle->WaitUntilFinished();
    if (handle->GetStatus() != Aws::Transfer::TransferStatus::COMPLETED) {
      return rocksdb::Status::IOError("Download failed for key: " + s3_key);
    }

    // LOG(INFO) << "[SstDownloader] Downloaded " << s3_key
    //           << " -> " << local_path.string();
    out_sst_files.push_back(local_path.string());
  }

  return rocksdb::Status::OK();
}

