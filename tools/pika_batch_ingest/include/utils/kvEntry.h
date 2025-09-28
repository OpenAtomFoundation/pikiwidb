#ifndef KV_ENTRY_H
#define KV_ENTRY_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <random>
#include "utils/kconfig.h"

using json = nlohmann::json;

struct KvEntry
{
    std::string key;
    std::string value;
    uint32_t timestamp = 0;
};

using KvData = std::vector<KvEntry>;
using DataType = KvData;

inline void to_json(json &j, const KvEntry &entry)
{
    j = json{
        {"key", entry.key},
        {"value", entry.value}};

    if (entry.timestamp != 0)
    {
        j["expire"] = entry.timestamp;
    }
}

inline void from_json(const json &j, KvEntry &entry)
{
    entry.key = j.at("key").get<std::string>();
    entry.value = j.at("value").get<std::string>();
    entry.timestamp = j.value("expire", 0); 
}

inline uint32_t generateRandomTimestamp(double zero_prob = 0.5, int offsetSeconds = 3600)
{
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<int> offset_dist(-offsetSeconds, offsetSeconds);

    if (prob_dist(gen) < zero_prob)
    {
        return 0; 
    }

    uint32_t now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    return now + offset_dist(gen);
}

#endif // KV_ENTRY_H
