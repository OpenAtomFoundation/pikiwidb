#ifndef S3UPLOADER_H
#define S3UPLOADER_H

#include <string>
#include <memory>
#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include "utils/result.h"

namespace s3put
{

    class S3Uploader
    {
    public:
        S3Uploader(const std::string &config_path); 
        ~S3Uploader();

        Result UploadFile(const std::string &local_path, const std::string &s3_key, const std::string &bucket = "");
        Result UploadText(const std::string &content, const std::string &s3_key, const std::string &bucket = "");
        Result setS3Client(const std::shared_ptr<Aws::S3::S3Client> &client);

    private:
        Aws::SDKOptions options_;
        std::shared_ptr<Aws::S3::S3Client> client_;
        std::string bucket_;
    };

}

#endif // S3UPLOADER_H