#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace utils {

enum class LogLevel {
    INFO,
    ERROR
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void init(const std::string& logFile, LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(mutex_);
        logLevel_ = level;
        if (!logFile.empty()) {
            fileStream_.open(logFile, std::ios::out | std::ios::app);
            if (!fileStream_.is_open()) {
                std::cerr << "Failed to open log file: " << logFile << std::endl;
            }
        }
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

    ~Logger() {
        if (fileStream_.is_open()) {
            fileStream_.close();
        }
    }

private:
    Logger() = default;

    void log(LogLevel level, const std::string& message) {
        if (level < logLevel_) {
            return;
        }

        std::string levelStr = level == LogLevel::INFO ? "INFO" : "ERROR";
        std::string timestamp = getCurrentTimestamp();

        std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message + "\n";

        std::lock_guard<std::mutex> lock(mutex_);
        
        // 输出到控制台
        if (level == LogLevel::ERROR) {
            std::cerr << logMessage;
        } else {
            std::cout << logMessage;
        }

        // 输出到文件
        if (fileStream_.is_open()) {
            fileStream_ << logMessage;
            fileStream_.flush();
        }
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_time);
        
        std::stringstream ss;
        ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        
        // 添加毫秒
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch() % std::chrono::seconds(1)).count();
        ss << "." << std::setw(3) << std::setfill('0') << ms;
        
        return ss.str();
    }

private:
    std::ofstream fileStream_;
    LogLevel logLevel_ = LogLevel::INFO;
    std::mutex mutex_;
};

// 全局日志宏
#define LOG_INFO(message) Logger::getInstance().info(message)
#define LOG_ERROR(message) Logger::getInstance().error(message)

} // namespace utils
