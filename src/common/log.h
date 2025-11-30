#ifndef LOG_H
#define LOG_H

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Log {
public:
    Log() = default;
    ~Log();

    // 禁止拷贝和移动
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log&&) = delete;

    // 单例模式
    static Log& Instance();

    // 初始化日志系统
    bool Init(const std::string& log_file_path, LogLevel level = LogLevel::INFO);

    // 日志输出函数
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warning(const std::string& message);
    void Error(const std::string& message);
    void Fatal(const std::string& message);

private:
    // 实际日志输出函数
    void LogMessage(LogLevel level, const std::string& message);

    // 将日志级别转换为字符串
    std::string LevelToString(LogLevel level) const;

    // 获取当前时间的字符串表示
    std::string GetCurrentTime() const;

private:
    std::ofstream log_file_;
    LogLevel log_level_ = LogLevel::INFO;
    std::mutex log_mutex_;
    bool is_initialized_ = false;
};

// 日志宏定义
#define LOG_DEBUG(msg) Log::Instance().Debug(msg)
#define LOG_INFO(msg) Log::Instance().Info(msg)
#define LOG_WARNING(msg) Log::Instance().Warning(msg)
#define LOG_ERROR(msg) Log::Instance().Error(msg)
#define LOG_FATAL(msg) Log::Instance().Fatal(msg)

#endif // LOG_H
