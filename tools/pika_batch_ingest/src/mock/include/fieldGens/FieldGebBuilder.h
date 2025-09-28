#ifndef FIELDGENBUILDER_H
#define FIELDGENBUILDER_H

#include "fieldGens/IFieldGenerator.h"
#include "fieldGens/RandomFieldGenerator.h"
#include "fieldGens/ZipfianFieldGenerator.h"
#include "fieldGens/NormalFieldGenerator.h"
#include "fieldGens/UniformFieldGenerator.h"
#include "fieldGens/SafeFieldGenerator.h"
#include "utils/klog.h"
#include <memory>

namespace mock
{
    enum class FieldDistributionType
    {
        Random,
        Uniform,
        Zipfian,
        Normal
    };

    inline std::shared_ptr<IFieldGenerator> createFieldGenerator(FieldDistributionType type,
                                                                 const std::string &fieldPrefix,
                                                                 size_t fieldSize,
                                                                 size_t poolSize)
    {
        std::shared_ptr<IFieldGenerator> baseGen;

        switch (type)
        {
        case FieldDistributionType::Uniform:
            LOG_DEBUG("Creating UniformFieldGenerator with prefix: " + fieldPrefix);
            baseGen = std::make_shared<UniformFieldGenerator>();
            break;
        case FieldDistributionType::Zipfian:
            LOG_DEBUG("Creating ZipfianFieldGenerator with prefix: " + fieldPrefix);
            baseGen = std::make_shared<ZipfianFieldGenerator>();
            break;
        case FieldDistributionType::Normal:
            LOG_DEBUG("Creating NormalFieldGenerator with prefix: " + fieldPrefix);
            baseGen = std::make_shared<NormalFieldGenerator>();
            break;
        case FieldDistributionType::Random:
        default:
            LOG_DEBUG("Creating RandomFieldGenerator with prefix: " + fieldPrefix);
            baseGen = std::make_shared<RandomFieldGenerator>();
            break;
        }

        baseGen->setContext(fieldPrefix, poolSize, fieldSize);
        return std::make_shared<SafeFieldGenerator>(baseGen);
    }

    inline FieldDistributionType parseDistribution(const std::string &dist)
    {
        if (dist == "uniform")
            return mock::FieldDistributionType::Uniform;
        else if (dist == "zipfian")
            return mock::FieldDistributionType::Zipfian;
        else if (dist == "normal")
            return mock::FieldDistributionType::Normal;
        else
            return mock::FieldDistributionType::Random;
    }

} // namespace mock

#endif // FIELDGENBUILDER_H