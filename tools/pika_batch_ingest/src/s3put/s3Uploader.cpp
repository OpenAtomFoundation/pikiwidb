#include "s3Uploader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <aws/core/utils/memory/stl/SimpleStringStream.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/CreateBucketConfiguration.h>
#include "configManager.h"
#include "utils/klog.h"

namespace s3put
{

    S3Uploader::S3Uploader(const std::string &config_path)
    {
        if (!ConfigManager::getInstance().loadConfig(config_path))
        {
            LOG_ERROR("Failed to load config file: " + config_path);
            return;
        }

        LOG_DEBUG("Initializing S3Uploader with config: " + config_path);

        Aws::InitAPI(options_);
        LOG_DEBUG("AWS SDK initialized");
        Aws::Client::ClientConfiguration cfg;
        cfg.region = ConfigManager::getInstance().getConfigValue<std::string>("region");
        cfg.endpointOverride = ConfigManager::getInstance().getConfigValue<std::string>("endpoint");
        cfg.scheme = (cfg.endpointOverride.find("https") == 0) ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
        cfg.verifySSL = false; 

        bool is_minio = ConfigManager::getInstance().getConfigValue<bool>("is_minio");

        Aws::Auth::AWSCredentials creds(
            ConfigManager::getInstance().getConfigValue<std::string>("access_key"),
            ConfigManager::getInstance().getConfigValue<std::string>("secret_key"));

        client_ = std::make_shared<Aws::S3::S3Client>(
            creds,
            cfg,
            Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
            !is_minio,
            Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::LEGACY);

        LOG_DEBUG("S3 client initialized with endpoint: " + cfg.endpointOverride);
        bucket_ = ConfigManager::getInstance().getConfigValue<std::string>("bucket");

        Aws::S3::Model::HeadBucketRequest head_bucket_request;
        head_bucket_request.SetBucket(bucket_);

        auto head_outcome = client_->HeadBucket(head_bucket_request);

        if (!head_outcome.IsSuccess())
        {
            auto code = head_outcome.GetError().GetErrorType();
            LOG_WARN("Bucket not found, will attempt to create: " + bucket_);

            Aws::S3::Model::CreateBucketRequest create_request;
            create_request.SetBucket(bucket_);

            if (!is_minio)
            {
                Aws::S3::Model::CreateBucketConfiguration config;
                config.SetLocationConstraint(Aws::S3::Model::BucketLocationConstraintMapper::GetBucketLocationConstraintForName(cfg.region));
                create_request.SetCreateBucketConfiguration(config);
            }

            auto create_outcome = client_->CreateBucket(create_request);
            if (create_outcome.IsSuccess())
            {
                LOG_INFO("Bucket created successfully: " + bucket_);
            }
            else
            {
                LOG_ERROR("Failed to create bucket: " + create_outcome.GetError().GetMessage());
            }
        }
        else
        {
            LOG_INFO("Bucket already exists: " + bucket_);
        }
    }

    S3Uploader::~S3Uploader()
    {
        Aws::ShutdownAPI(options_); 
    }

    Result S3Uploader::UploadFile(const std::string &local_path, const std::string &s3_key, const std::string &bucket)
    {
        const int max_retries = 3;
        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            Aws::S3::Model::PutObjectRequest request;
            const std::string &final_bucket = bucket.empty() ? bucket_ : bucket;
            request.SetBucket(final_bucket);
            LOG_DEBUG("Uploading file to Bucket: " + final_bucket);
            LOG_DEBUG("file name: " + s3_key);
            request.SetKey(s3_key);

            auto input_data = Aws::MakeShared<Aws::FStream>(
                "UploadTag", local_path.c_str(), std::ios_base::in | std::ios_base::binary);

            if (!input_data->good()) {
                LOG_ERROR("Failed to open local file: " + local_path);
                return Result(Result::Ret::kFileOpenError, "Failed to open local file: " + local_path);
            }

            request.SetBody(input_data);
            auto outcome = client_->PutObject(request);

            if (outcome.IsSuccess()) {
                LOG_DEBUG("Uploaded: " + s3_key);
                return Result(Result::Ret::kOk, "Uploaded: " + s3_key);
            } else {
                std::string error_msg = "S3 upload failed: " + outcome.GetError().GetMessage();
                if (attempt < max_retries) {
                    LOG_WARN("Upload attempt " + std::to_string(attempt + 1) + " failed: " + error_msg + ". Retrying...");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1))); 
                } else {
                    LOG_ERROR(error_msg);
                    return Result(Result::Ret::kS3UploadError, error_msg);
                }
            }
        }
        return Result(Result::Ret::kS3UploadError, "Upload failed after retries");
    }

    Result S3Uploader::UploadText(const std::string &content, const std::string &s3_key, const std::string &bucket)
    {
        const int max_retries = 3;
        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            Aws::S3::Model::PutObjectRequest request;
            const std::string &final_bucket = bucket.empty() ? bucket_ : bucket;
            request.SetBucket(final_bucket);
            LOG_DEBUG("Uploading text to Bucket: " + final_bucket);
            request.SetKey(s3_key);

            auto stream = Aws::MakeShared<Aws::StringStream>("UploadTextTag");
            *stream << content;
            request.SetBody(stream);

            auto outcome = client_->PutObject(request);
            if (outcome.IsSuccess()) {
                LOG_DEBUG("Uploaded text to: " + s3_key);
                return Result(Result::Ret::kOk, "Uploaded text to: " + s3_key);
            } else {
                std::string error_msg = "Upload text failed: " + outcome.GetError().GetMessage();
                if (attempt < max_retries) {
                    LOG_WARN("Upload text attempt " + std::to_string(attempt + 1) + " failed: " + error_msg + ". Retrying...");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1)));
                } else {
                    LOG_ERROR(error_msg);
                    return Result(Result::Ret::kS3UploadError, error_msg);
                }
            }
        }
        return Result(Result::Ret::kS3UploadError, "Upload text failed after retries");
    }

    Result S3Uploader::setS3Client(const std::shared_ptr<Aws::S3::S3Client> &client)
    {
        if (!client)
        {
            LOG_ERROR("Provided S3 client is null");
            return Result(Result::Ret::kInvalidArgument, "Provided S3 client is null");
        }
        client_ = client;
        LOG_DEBUG("S3 client set successfully");
        return Result(Result::Ret::kOk, "S3 client set successfully");
    }
}