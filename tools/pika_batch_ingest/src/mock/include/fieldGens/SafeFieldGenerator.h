#ifndef SAFE_FIELD_GENERATOR_H
#define SAFE_FIELD_GENERATOR_H

#include "fieldGens/IFieldGenerator.h"
#include <shared_mutex>
#include <memory>
#include <shared_mutex>

namespace mock
{
    class SafeFieldGenerator : public IFieldGenerator
    {
    public:
        explicit SafeFieldGenerator(std::shared_ptr<IFieldGenerator> wrapped)
            : wrapped_(std::move(wrapped)) {}

        Result generateField() override
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return wrapped_->generateField();
        }

        size_t getFieldPoolSize() override
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return wrapped_->getFieldPoolSize();
        }

        size_t getFieldSize() override
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return wrapped_->getFieldSize();
        }

        size_t generateIndex() override
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return wrapped_->generateIndex();
        }

    private:
        std::shared_ptr<IFieldGenerator> wrapped_;
        mutable std::shared_mutex mutex_;
    };

} // namespace mock

#endif // SAFE_FIELD_GENERATOR_H