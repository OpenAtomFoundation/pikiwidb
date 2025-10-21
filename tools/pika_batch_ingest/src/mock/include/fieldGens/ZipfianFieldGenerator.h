#ifndef ZIPFIANFIELDGENERATOR_H
#define ZIPFIANFIELDGENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include "utils/result.h"
#include "utils/klog.h"
#include <random>
#include <numeric>
#include <cmath>

namespace mock
{
    class ZipfianFieldGenerator : public IFieldGenerator
    {
    public:
        ZipfianFieldGenerator(double skew = 1.0) : skew_(skew) {}

        Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16) override
        {
            return IFieldGenerator::setContext(fieldPrefix, poolSize, fieldSize);
        }

        size_t generateIndex() override
        {
            ensureDistributionInitialized();
            thread_local std::mt19937 gen(std::random_device{}());
            thread_local std::discrete_distribution<size_t> dist(zipfWeights_.begin(), zipfWeights_.end());
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

    private:
        void ensureDistributionInitialized()
        {
            if (!distributionInitialized_ && logicalPool_.size > 0)
            {
                zipfWeights_.resize(logicalPool_.size);
                double sum = 0.0;
                for (size_t i = 1; i <= logicalPool_.size; ++i)
                {
                    zipfWeights_[i - 1] = 1.0 / std::pow(static_cast<double>(i), skew_);
                    sum += zipfWeights_[i - 1];
                }
                for (auto &w : zipfWeights_)
                {
                    w /= sum;
                }
                distributionInitialized_ = true;
                LOG_DEBUG("Initialized Zipfian weights with skew: " + std::to_string(skew_));
            }
        }

        double skew_ = 1.0;
        bool distributionInitialized_ = false;
        std::vector<double> zipfWeights_;
    };

} // namespace mock

#endif // ZIPFIANFIELDGENERATOR_H