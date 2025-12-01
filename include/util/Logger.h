#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace util {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& file_path);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
private:
    Logger();
    ~Logger();
    
    void log(LogLevel level, const std::string& message);
    std::string getLogLevelString(LogLevel level);
    std::string getCurrentTimestamp();
    
    LogLevel log_level_ = LogLevel::INFO;
    std::ofstream log_file_;
    std::mutex mutex_;
    bool use_console_ = true;
};

// 便捷宏
#define LOG_DEBUG(msg) util::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) util::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) util::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) util::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) util::Logger::getInstance().critical(msg)

} // namespace util
