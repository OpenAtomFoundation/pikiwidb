#ifndef RANDOMFIELDGENERATOR_H
#define RANDOMFIELDGENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include "utils/result.h"
#include "utils/klog.h"
#include <random>
#include <thread>

namespace mock
{

    class RandomFieldGenerator : public IFieldGenerator
    {
    public:
        RandomFieldGenerator() = default;

        Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16) override
        {
            return IFieldGenerator::setContext(fieldPrefix, poolSize, fieldSize);
        }

        size_t generateIndex() override
        {
            thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<size_t> dist(0, logicalPool_.size - 1);
            return dist(gen);
        }

        size_t getFieldPoolSize() override
        {
            return logicalPool_.size;
        }

        size_t getFieldSize() override
        {
            return fieldSize_;
        }
    };

} // namespace mock

#endif // RANDOMFIELDGENERATOR_H