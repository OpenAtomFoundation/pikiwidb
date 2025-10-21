#ifndef RESULT_H
#define RESULT_H
#include <string>

class Result
{
public:
    enum Ret
    {
        kNone = 0,         // 默认状态
        kOk,               // 成功
        kFileCreated,      // 文件创建成功
        kFileReadError,    // 文件读取错误
        kDataGenerated,    // 数据成功生成
        kError,            // 一般错误
        kFileOpenError,    // 文件打开错误
        kInvalidSize,      // 无效的文件大小
        kInvalidParam,     // 无效的参数
        kFileWriteError,   // 文件写入错误
        kOutOfMemory,      // 内存不足
        kFileTooLarge,     // 文件太大
        kInvalidUnit,      // 无效的单位（B, K, M, G）
        kInvalidRange,     // 超出允许的范围
        kInvalidFileType,  // 无效的文件类型
        kDataSizeMismatch, // 数据大小不匹配
        kProcessing,       // 数据生成中
        kCancelled,        // 操作被取消
        kConfigError,      // 配置错误
        kS3UploadError,    // S3上传错误
        kInvalidArgument   // 无效的参数
    };
    Result() = default;
    Result(Ret ret, const std::string &message = "")
        : ret_(ret), message_(message) {}

    bool isError() const
    {
        return ret_ != kOk && ret_ != kFileCreated && ret_ != kDataGenerated;
    }
    std::string message_raw() const { return message_; }

    std::string message() const
    {
        std::string result;
        switch (ret_)
        {
        case kNone:
            return message_;
        case kOk:
            return "+OK\r\n";
        case kFileCreated:
            return "+OK File created successfully\r\n";
        case kDataGenerated:
            return "+OK Data generated successfully\r\n";
        case kError:
            return "-ERR An unknown error occurred\r\n";
        case kFileOpenError:
            result = "-ERR Unable to open file for '";
            result.append(message_);
            result.append("'\r\n");
        case kFileReadError:
            result = "-ERR Error reading file for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kInvalidSize:
            result = "-ERR Invalid size parameter for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kInvalidParam:
            result = "-ERR Invalid parameters provided for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kFileWriteError:
            result = "-ERR Error writing to file for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kOutOfMemory:
            result = "-ERR Out of memory for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kFileTooLarge:
            result = "-ERR File size exceeds the allowed limit for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kInvalidUnit:
            result = "-ERR Invalid unit (use B, K, M, G) for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kInvalidRange:
            result = "-ERR The specified range is invalid for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kInvalidFileType:
            result = "-ERR Invalid file type (only .json supported) for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kDataSizeMismatch:
            result = "-ERR The generated data size does not match the expected size for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kProcessing:
            result = "-ERR Data generation in progress for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kCancelled:
            result = "-ERR Operation was cancelled for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kConfigError:
            result = "-ERR Configuration error for '";
            result.append(message_);
            result.append("'\r\n");
            return result;
        case kS3UploadError:
            result = "-ERR S3Upload error for '";
            result.append(message_);
            result.append("'\r\n");
        case kInvalidArgument:
            result = "-ERR Invalid Argument error for '";
            result.append(message_);
            result.append("'\r\n");
        default:
            return "-ERR Unknown status.\r\n";
        }
    }

    void setRes(Ret ret, const std::string &content = "")
    {
        ret_ = ret;
        if (!content.empty())
        {
            message_ = content;
        }
    }

    Ret getRet() const { return ret_; }

private:
    Ret ret_ = kNone;
    std::string message_;
};

#endif
