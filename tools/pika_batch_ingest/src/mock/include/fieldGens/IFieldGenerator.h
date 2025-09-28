#ifndef IFIELDGENERATOR_H
#define IFIELDGENERATOR_H
#include <string>
#include <vector>
#include "utils/result.h"
#include <thread>
#include <random>

namespace mock
{
    struct LogicalFieldPool
    {
        std::string prefix;
        size_t size;
    };

    class IFieldGenerator
    {
    public:
        virtual ~IFieldGenerator() = default;
        IFieldGenerator() = default;

        virtual size_t getFieldPoolSize() = 0;
        virtual size_t getFieldSize() = 0;
        virtual size_t generateIndex() = 0;
        virtual Result generateField()
        {
            std::string full = logicalPool_.prefix; 

            while (full.size() < fieldSize_)
            {
                size_t index = generateIndex();
                std::string indexStr = std::to_string(index);

                if (full.size() + indexStr.size() >= fieldSize_)
                {
                    size_t remain = fieldSize_ - full.size();
                    full += indexStr.substr(0, remain); 
                    break;
                }

                full += indexStr;
            }

            return Result(Result::kOk, full);
        }

        virtual size_t estimateRepeatCount() const
        {
            size_t indexDigits = std::to_string(logicalPool_.size).size();
            size_t avgSegmentLength = logicalPool_.prefix.size() + indexDigits;
            return (fieldSize_ + avgSegmentLength - 1) / avgSegmentLength;
        }

        virtual Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16)
        {
            if (fieldPrefix.empty())
            {
                return Result(Result::Ret::kError, "Field prefix cannot be empty.");
            }
            if (poolSize == 0)
            {
                return Result(Result::Ret::kError, "Pool size must be greater than 0.");
            }
            if (fieldSize == 0 || fieldSize > 1024)
            {
                return Result(Result::Ret::kError, "Field size must be in range [1, 1024].");
            }
            logicalPool_ = LogicalFieldPool{fieldPrefix, poolSize};
            fieldSize_ = fieldSize;
            return Result(Result::Ret::kOk, "Context set successfully.");
        }

    protected:
        LogicalFieldPool logicalPool_{}; 
        size_t fieldSize_ = 16;      
    };

} // namespace mock

#endif // IFIELDGENERATOR_H
