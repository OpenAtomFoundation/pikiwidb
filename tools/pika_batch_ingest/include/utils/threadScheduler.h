#pragma once
#include <mutex>
#include <thread>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <fstream>
#include "utils/klog.h"
#include <nlohmann/json.hpp>

class ThreadScheduler
{
public:
    static ThreadScheduler &get()
    {
        static ThreadScheduler instance;
        return instance;
    }

    void init(unsigned totalThreads = std::thread::hardware_concurrency())
    {
        std::lock_guard<std::mutex> lock(mutex_);
        total_ = std::max(1u, (2 * totalThreads) / 3);
        used_ = 0;
        finalized_ = false;
        logicalRequests_.clear();
        allocations_.clear();
    }

    void registerLogicalRequest(const std::string &name, size_t logical)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (finalized_)
        {
            LOG_WARN("ThreadScheduler::registerLogicalRequest called after finalize(). Ignored.");
            return;
        }
        logicalRequests_[name] = logical;
    }

    void finalize()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t totalLogical = 0;
        for (const auto &pair : logicalRequests_)
        {
            totalLogical += pair.second;
        }

        if (totalLogical == 0)
        {
            LOG_INFO("No logical requests registered. Nothing to allocate.");
            return;
        }

        LOG_DEBUG("========== ThreadScheduler Finalization ==========");
        LOG_DEBUG("Total usable threads: " + std::to_string(total_));
        LOG_DEBUG("Total logical request weight: " + std::to_string(totalLogical));

        // 按权重比例分配线程
        std::vector<std::pair<std::string, size_t>> modules;
        for (const auto &pair : logicalRequests_) {
            modules.push_back({pair.first, pair.second});
        }
        
        // 按权重从大到小排序，优先分配权重高的模块
        std::sort(modules.begin(), modules.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // 按权重比例分配线程，但确保每个模块至少分配1个线程（除非没有线程可用）
        size_t allocatedCount = 0;
        for (const auto &module : modules) {
            const std::string &name = module.first;
            size_t logical = module.second;
            
            // 计算按比例应分配的线程数
            size_t allocated = (total_ * logical) / totalLogical;
            
            // 确保至少分配1个线程（除非total_为0或已分配完所有线程）
            if (allocated == 0 && total_ > 0 && used_ < total_) {
                allocated = 1;
            }
            
            // 如果需要的线程数超过了剩余线程数，则只分配剩余的线程
            if (used_ + allocated > total_) {
                allocated = total_ - used_;
            }
            
            allocations_[name] = allocated;
            used_ += allocated;
            allocatedCount += allocated;
            
            LOG_DEBUG("Module: " + name + 
                      " | Logical Weight: " + std::to_string(logical) +
                      " | Allocated Threads: " + std::to_string(allocated));
        }
        
        // 如果还有剩余线程，按照权重重新分配给各模块
        if (used_ < total_) {
            size_t remaining = total_ - used_;
            LOG_DEBUG("Remaining threads to distribute: " + std::to_string(remaining));
            
            // 按权重比例分配剩余线程
            for (const auto &module : modules) {
                const std::string &name = module.first;
                size_t logical = module.second;
                
                size_t additional = (remaining * logical) / totalLogical;
                
                // 确保不超出剩余线程数
                if (additional > (total_ - used_)) {
                    additional = total_ - used_;
                }
                
                allocations_[name] += additional;
                used_ += additional;
                
                if (additional > 0) {
                    LOG_DEBUG("Module: " + name + 
                              " | Additional Threads: " + std::to_string(additional));
                }
                
                if (used_ >= total_) {
                    break;
                }
            }
        }

        LOG_DEBUG("Total threads used: " + std::to_string(used_));
        LOG_DEBUG("===================================================");

        finalized_ = true;
    }

    // 获取某个模块实际分配的线程数
    size_t get(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(name);
        if (it != allocations_.end())
        {
            return it->second;
        }
        LOG_WARN("ThreadScheduler::get called for unregistered module: " + name);
        return 1; // fallback
    }

    size_t available() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_ - used_;
    }

private:
    ThreadScheduler() = default;

    size_t total_ = 0;
    size_t used_ = 0;
    bool finalized_ = false;

    std::unordered_map<std::string, size_t> logicalRequests_;
    std::unordered_map<std::string, size_t> allocations_;

    mutable std::mutex mutex_;
};

inline void initThreadSchedulerFromConfig(const std::string &jsonPath)
{
    std::ifstream file(jsonPath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open thread config file: " + jsonPath + ". Using default settings.");
        return;
    }

    nlohmann::json conf;
    try
    {
        file >> conf;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Invalid JSON in thread config file: " + std::string(e.what()));
        return;
    }

    auto &sched = ThreadScheduler::get();
    sched.init(); // 初始化线程调度器

    if (!conf.is_object())
    {
        LOG_ERROR("Thread config file must contain a JSON object.");
        return;
    }

    for (auto &[name, weight] : conf.items())
    {
        if (weight.is_number_unsigned())
        {
            sched.registerLogicalRequest(name, weight.get<size_t>());
        }
        else
        {
            LOG_ERROR("Invalid thread weight for module: " + name);
        }
    }

    sched.finalize(); // 分配
}
