#ifndef NORMALFIELDGENERATOR_H
#define NORMALFIELDGENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include "utils/result.h"
#include "utils/klog.h"
#include <random>
#include <cmath>

namespace mock
{

    class NormalFieldGenerator : public IFieldGenerator
    {
    public:
        NormalFieldGenerator() = default;

        Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16) override
        {
            return IFieldGenerator::setContext(fieldPrefix, poolSize, fieldSize);
        }

        size_t generateIndex() override
        {
            thread_local std::mt19937 generator(std::random_device{}());
            thread_local std::normal_distribution<double> dist;

            if (!initialized_)
            {
                mean_ = static_cast<double>(logicalPool_.size) / 2.0;
                stddev_ = static_cast<double>(logicalPool_.size) / 6.0;
                dist = std::normal_distribution<double>(mean_, stddev_);
                initialized_ = true;
            }

            for (int retry = 0; retry < 10; ++retry)
            {
                double sampled = dist(generator);
                if (sampled >= 0 && sampled < static_cast<double>(logicalPool_.size))
                {
                    return static_cast<size_t>(sampled);
                }
            }

            return static_cast<size_t>(mean_);
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
        double mean_ = 0.0;  
        double stddev_ = 1.0; 
        bool initialized_ = false;
    };

} // namespace mock

#endif // NORMALFIELDGENERATOR_H