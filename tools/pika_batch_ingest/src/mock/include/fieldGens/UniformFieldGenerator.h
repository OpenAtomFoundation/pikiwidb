#ifndef UNIFORMFIELDGENERATOR_H
#define UNIFORMFIELDGENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include "utils/result.h"
#include "utils/klog.h"
#include <atomic>
#include <thread>

namespace mock
{
    class UniformFieldGenerator : public IFieldGenerator
    {
    public:
        UniformFieldGenerator() = default;
        Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16) override
        {
            return IFieldGenerator::setContext(fieldPrefix, poolSize, fieldSize);
        }

        size_t generateIndex() override
        {
            return counter_.fetch_add(1, std::memory_order_relaxed) % logicalPool_.size;
        }

        size_t getFieldPoolSize() override
        {
            return logicalPool_.size;
        }

        size_t getFieldSize() override
        {
            return fieldSize_;
        }

    private:
        std::atomic<size_t> counter_{0};
    };

} // namespace mock

#endif // UNIFORMFIELDGENERATOR_H
