#include "log.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>

Log::~Log() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

Log& Log::Instance() {
    static Log instance;
    return instance;
}

bool Log::Init(const std::string& log_file_path, LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (is_initialized_) {
        return true;
    }

    log_file_.open(log_file_path, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Failed to open log file: " << log_file_path << std::endl;
        return false;
    }

    log_level_ = level;
    is_initialized_ = true;

    LogMessage(LogLevel::INFO, "Log system initialized successfully");
    return true;
}

void Log::Debug(const std::string& message) {
    LogMessage(LogLevel::DEBUG, message);
}

void Log::Info(const std::string& message) {
    LogMessage(LogLevel::INFO, message);
}

void Log::Warning(const std::string& message) {
    LogMessage(LogLevel::WARNING, message);
}

void Log::Error(const std::string& message) {
    LogMessage(LogLevel::ERROR, message);
}

void Log::Fatal(const std::string& message) {
    LogMessage(LogLevel::FATAL, message);
}

void Log::LogMessage(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (!is_initialized_) {
        return;
    }

    if (level < log_level_) {
        return;
    }

    std::string log_line = GetCurrentTime() + " [" + LevelToString(level) + "] " + message;

    // 同时输出到文件和控制台
    log_file_ << log_line << std::endl;
    std::cout << log_line << std::endl;

    if (level == LogLevel::FATAL) {
        std::abort();
    }
}

std::string Log::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

std::string Log::GetCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}
