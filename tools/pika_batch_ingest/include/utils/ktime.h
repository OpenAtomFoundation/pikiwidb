#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include "utils/kconfig.h"

namespace fs = std::filesystem;

class TimeTracker
{
public:
    static void Start(const std::string &actionName)
    {
        action_ = actionName;
        start_time_ = std::chrono::steady_clock::now();
        std::string log_message = action_ + "[TIME] Starting " + " at " + GetCurrentTime();
        std::cout << log_message << std::endl;
        WriteToCSV(log_message); 
    }

    static void End()
    {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        std::string log_message = action_ + "[TIME] completed in " + std::to_string(duration.count()) + " ms.";
        std::cout << log_message << std::endl;
        WriteToCSV(log_message); 
    }

private:
    static std::chrono::steady_clock::time_point start_time_;
    static std::string action_;
    static std::string GetCurrentTime()
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string time_str = std::ctime(&now);
        time_str.pop_back(); 
        return time_str;
    }

    static void WriteToCSV(const std::string &message)
    {
        fs::path parentDir = fs::path(TIEMRECORDPATH).parent_path();
        if (!parentDir.empty() && !fs::exists(parentDir))
        {
            fs::create_directories(parentDir);
        }

        std::ofstream csv_file(TIEMRECORDPATH, std::ios::app); 
        if (csv_file.is_open())
        {
            if (csv_file.tellp() == 0)
            {
                csv_file << "Log Message\n"; 
            }

            csv_file << message << "\n";
            csv_file.close();
        }
        else
        {
            std::cerr << "Failed to open CSV file for writing." << std::endl;
        }
    }
};

std::chrono::steady_clock::time_point TimeTracker::start_time_;
std::string TimeTracker::action_;
