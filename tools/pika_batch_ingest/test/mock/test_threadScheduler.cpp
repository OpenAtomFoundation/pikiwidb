#include <gtest/gtest.h>
#include "utils/threadScheduler.h"
#include <nlohmann/json.hpp>

class ThreadSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& scheduler = ThreadScheduler::get();
    }
};

TEST_F(ThreadSchedulerTest, Singleton) {
    auto& scheduler1 = ThreadScheduler::get();
    auto& scheduler2 = ThreadScheduler::get();
    EXPECT_EQ(&scheduler1, &scheduler2) << "ThreadScheduler should be a singleton";
}

TEST_F(ThreadSchedulerTest, Initialization) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(4); 
}

TEST_F(ThreadSchedulerTest, RegisterLogicalRequest) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(4);
    
    scheduler.registerLogicalRequest("dataGen", 2);
    scheduler.registerLogicalRequest("keyGen", 1);
    scheduler.registerLogicalRequest("valueGen", 1);
    SUCCEED();
}

TEST_F(ThreadSchedulerTest, ThreadAllocation) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(4);
    
    scheduler.registerLogicalRequest("dataGen", 2);
    scheduler.registerLogicalRequest("keyGen", 1);
    scheduler.registerLogicalRequest("valueGen", 1);
    
    scheduler.finalize();
    size_t dataGenThreads = scheduler.get("dataGen");
    size_t keyGenThreads = scheduler.get("keyGen");
    size_t valueGenThreads = scheduler.get("valueGen");
    EXPECT_LE(dataGenThreads + keyGenThreads + valueGenThreads, 4);
    
    EXPECT_GE(dataGenThreads, 0);
    EXPECT_GE(keyGenThreads, 0);
    EXPECT_GE(valueGenThreads, 0);
}

TEST_F(ThreadSchedulerTest, GetUnregisteredModule) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(2);
    scheduler.finalize();
    size_t threads = scheduler.get("unregistered_module");
    EXPECT_EQ(threads, 1);
}

TEST_F(ThreadSchedulerTest, ZeroThreadInitialization) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(0);
    scheduler.finalize();
    size_t available = scheduler.available();
}

TEST_F(ThreadSchedulerTest, LargeLogicalRequests) {
    auto& scheduler = ThreadScheduler::get();
    scheduler.init(8);
    scheduler.registerLogicalRequest("module1", 10);
    scheduler.registerLogicalRequest("module2", 5);
    scheduler.registerLogicalRequest("module3", 3);
    scheduler.registerLogicalRequest("module4", 2);
    scheduler.registerLogicalRequest("module5", 1);
    
    scheduler.finalize();
    size_t totalAllocated = 
        scheduler.get("module1") + 
        scheduler.get("module2") + 
        scheduler.get("module3") + 
        scheduler.get("module4") + 
        scheduler.get("module5");
    
    EXPECT_LE(totalAllocated, 8);
}