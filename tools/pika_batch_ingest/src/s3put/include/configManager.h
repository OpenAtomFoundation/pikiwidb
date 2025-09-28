#ifndef S3PUT_CONFIGMANAGER_H
#define S3PUT_CONFIGMANAGER_H
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>
#include "utils/klog.h"
namespace s3put
{

    class ConfigManager
    {
    public:
        static ConfigManager &getInstance()
        {
            static ConfigManager instance;
            return instance;
        }

        bool loadConfig(const std::string &config_path)
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            if (config_loaded_)
                return true;

            std::ifstream in(config_path);
            if (!in.is_open())
            {
                LOG_ERROR("Failed to open config file: " + config_path);
                return false;
            }

            in >> config_data_;
            config_loaded_ = true;

            return true;
        }

        template <typename T>
        T getConfigValue(const std::string &key) const
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            if (config_data_.contains(key))
            {
                return config_data_[key].get<T>();
            }
            else
            {
                throw std::runtime_error("Config key not found: " + key);
            }
        }

    private:
        ConfigManager() = default;

        nlohmann::json config_data_;
        mutable std::mutex config_mutex_;
        bool config_loaded_ = false;
    };

}
#endif // S3PUT_CONFIGMANAGER_H