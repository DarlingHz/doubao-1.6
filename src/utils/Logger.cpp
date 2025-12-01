#include "utils/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <fstream>

namespace utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : currentLevel(LogLevel::INFO), consoleOutput(true) {
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "无法打开日志文件: " << filePath << std::endl;
    }
}

void Logger::enableConsoleOutput(bool enable) {
    consoleOutput = enable;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) {
        return;
    }

    std::string logMessage = getTimestamp() + " [" + levelToString(level) + "] " + message;

    std::lock_guard<std::mutex> lock(logMutex);
    
    if (consoleOutput) {
        // 根据日志级别设置输出颜色
        std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        switch (level) {
            case LogLevel::DEBUG:
                out << "\033[36m" << logMessage << "\033[0m" << std::endl; // 青色
                break;
            case LogLevel::INFO:
                out << "\033[32m" << logMessage << "\033[0m" << std::endl; // 绿色
                break;
            case LogLevel::WARNING:
                out << "\033[33m" << logMessage << "\033[0m" << std::endl; // 黄色
                break;
            case LogLevel::ERROR:
                out << "\033[31m" << logMessage << "\033[0m" << std::endl; // 红色
                break;
            case LogLevel::FATAL:
                out << "\033[35m" << logMessage << "\033[0m" << std::endl; // 紫色
                break;
            default:
                out << logMessage << std::endl;
                break;
        }
    }

    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
        logFile.flush();
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_now;
    #ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
    #else
        localtime_r(&time_t_now, &tm_now);
    #endif

    std::stringstream ss;
    ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")
       << "." << std::setw(3) << std::setfill('0') << milliseconds.count();
    
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) const {
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

} // namespace utils