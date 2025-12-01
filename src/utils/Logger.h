#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filePath);
    void enableConsoleOutput(bool enable);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message);
    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;

    LogLevel currentLevel;
    std::ofstream logFile;
    bool consoleOutput;
    std::mutex logMutex;
};

} // namespace utils

// 日志宏定义
#define LOG_DEBUG(msg) utils::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) utils::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) utils::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) utils::Logger::getInstance().error(msg)
#define LOG_FATAL(msg) utils::Logger::getInstance().fatal(msg)

#endif // LOGGER_H