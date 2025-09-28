#ifndef PREFIXZIPFFIELDGENERATOR_H
#define PREFIXZIPFFIELDGENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include <random>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <thread>

namespace mock
{
    class PrefixZipfFieldGenerator : public IFieldGenerator
    {
    public:
        PrefixZipfFieldGenerator(double skew = 1.0, size_t maxSuffix = 99999)
            : skew_(skew), maxSuffix_(maxSuffix) {}

        Result setContext(const std::string &fieldPrefix, size_t poolSize, size_t fieldSize = 16) override
        {
            auto r = IFieldGenerator::setContext(fieldPrefix, poolSize, fieldSize);
            distributionInitialized_ = false;
            return r;
        }

        size_t generateIndex() override
        {
            thread_local std::mt19937 gen(std::random_device{}());
            ensureDistributionInitialized();

            size_t prefixIndex = prefixDist_(gen);
            size_t suffix = std::uniform_int_distribution<size_t>(0, maxSuffix_)(gen);

            std::ostringstream oss;
            oss << logicalPool_.prefix << prefixIndex << "_" << suffix;
            currentField_ = oss.str();
            return 0; 
        }

        Result generateField() override
        {
            generateIndex();
            std::string full = currentField_;

            if (full.size() >= fieldSize_)
                return Result(Result::Ret::kOk, full.substr(0, fieldSize_));
            else
                return Result(Result::Ret::kOk, full + std::string(fieldSize_ - full.size(), 'X'));
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
            if (distributionInitialized_ || logicalPool_.size == 0)
                return;

            std::vector<double> weights(logicalPool_.size);
            double sum = 0.0;
            for (size_t i = 1; i <= logicalPool_.size; ++i)
            {
                weights[i - 1] = 1.0 / std::pow(static_cast<double>(i), skew_);
                sum += weights[i - 1];
            }
            for (auto &w : weights)
                w /= sum;

            prefixDist_ = std::discrete_distribution<size_t>(weights.begin(), weights.end());
            distributionInitialized_ = true;
        }

        double skew_;
        bool distributionInitialized_ = false;
        std::discrete_distribution<size_t> prefixDist_;
        size_t maxSuffix_;
        std::string currentField_;
    };

} // namespace mock

#endif // PREFIXZIPFFIELDGENERATOR_H