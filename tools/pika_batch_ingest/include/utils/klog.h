#ifndef KLOG_H
#define KLOG_H

#include "kconfig.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include "kconfig.h"

class KLogger {
public:
    enum KLogLevel { Debug = 0, Info, Warning, Error };

    KLogger()
        : filename_(KLOGPATH) {
        if (!std::filesystem::exists(KLOGDICT)) {
            std::filesystem::create_directories(KLOGDICT);
        }
    }

    void setKLog(KLogLevel level, const std::string &msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
        message_ = msg;
    }

    void logMessage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (level_) {
#ifdef DEBUG
        case Debug:   std::cout << "[DEBUG] " << message_ << std::endl; break;
#endif
        case Info:    std::cout << "[INFO] " << message_ << std::endl; break;
        case Warning: std::cout << "[WARNING] " << message_ << std::endl; break;
        case Error:   std::cerr << "[ERROR] " << message_ << std::endl; break;
        }
    }

    void logToFile() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream logFile(filename_, std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);

            std::tm tm_buf;
#ifdef _WIN32
            localtime_s(&tm_buf, &now_time);
#else
            localtime_r(&now_time, &tm_buf);
#endif
            logFile << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << " ";

            switch (level_) {
#ifdef DEBUG
            case Debug:   logFile << "[DEBUG] "; break;
#endif
            case Info:    logFile << "[INFO] "; break;
            case Warning: logFile << "[WARNING] "; break;
            case Error:   logFile << "[ERROR] "; break;
            }
            logFile << message_ << std::endl;
        }
    }

private:
    mutable std::mutex mutex_;
    KLogLevel level_ = Info;
    std::string message_;
    std::filesystem::path filename_;
};

inline KLogger klogger;

#define KLOG(level, content)                             \
    klogger.setKLog(KLogger::KLogLevel::level, content); \
    klogger.logMessage();                                \
    klogger.logToFile()

#define LOG_DEBUG(content) KLOG(Debug, content)
#define LOG_INFO(content) KLOG(Info, content)
#define LOG_WARN(content) KLOG(Warning, content)
#define LOG_ERROR(content)                                                     \
  KLOG(Error, content);                                                        \
  exit(EXIT_FAILURE)

#endif // KLOG_H
