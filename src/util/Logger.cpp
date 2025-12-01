#include <util/Logger.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

namespace util {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // 默认只输出到控制台
    use_console_ = true;
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    log_level_ = level;
}

void Logger::setLogFile(const std::string& file_path) {
    if (log_file_.is_open()) {
        log_file_.close();
    }
    
    log_file_.open(file_path, std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "无法打开日志文件: " << file_path << std::endl;
        use_console_ = true;
    } else {
        use_console_ = false;
    }
}

void Logger::debug(const std::string& message) {
    if (log_level_ <= LogLevel::DEBUG) {
        log(LogLevel::DEBUG, message);
    }
}

void Logger::info(const std::string& message) {
    if (log_level_ <= LogLevel::INFO) {
        log(LogLevel::INFO, message);
    }
}

void Logger::warning(const std::string& message) {
    if (log_level_ <= LogLevel::WARNING) {
        log(LogLevel::WARNING, message);
    }
}

void Logger::error(const std::string& message) {
    if (log_level_ <= LogLevel::ERROR) {
        log(LogLevel::ERROR, message);
    }
}

void Logger::critical(const std::string& message) {
    if (log_level_ <= LogLevel::CRITICAL) {
        log(LogLevel::CRITICAL, message);
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::string log_message = getCurrentTimestamp() + " [" + getLogLevelString(level) + "] " + message;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (use_console_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_message << std::endl;
        } else {
            std::cout << log_message << std::endl;
        }
    } else {
        if (log_file_.is_open()) {
            log_file_ << log_message << std::endl;
            log_file_.flush();
        }
    }
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm_now;
    #ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
    #else
        localtime_r(&time_t_now, &tm_now);
    #endif
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
    
    return ss.str();
}

} // namespace util
