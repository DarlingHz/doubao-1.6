// 日志工具类
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
private:
    static Logger* instance;
    std::ofstream logFile;
    std::mutex logMutex;
    LogLevel currentLevel;
    bool consoleOutput;
    
    Logger();
    ~Logger();
    
    // 将日志级别转换为字符串
    std::string levelToString(LogLevel level);
    
    // 获取当前时间的字符串表示
    std::string getCurrentTime();
    
public:
    static Logger* getInstance();
    
    // 初始化日志文件
    bool initialize(const std::string& logFilePath = "carpool.log");
    
    // 设置日志级别
    void setLogLevel(LogLevel level);
    
    // 设置是否输出到控制台
    void setConsoleOutput(bool output);
    
    // 日志记录方法
    void log(LogLevel level, const std::string& message);
    
    // 便捷方法
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // 关闭日志文件
    void shutdown();
};

// 日志宏定义，方便使用
#define LOG_DEBUG(msg) Logger::getInstance()->debug(msg)
#define LOG_INFO(msg) Logger::getInstance()->info(msg)
#define LOG_WARNING(msg) Logger::getInstance()->warning(msg)
#define LOG_ERROR(msg) Logger::getInstance()->error(msg)
#define LOG_CRITICAL(msg) Logger::getInstance()->critical(msg)

#endif // LOGGER_H
