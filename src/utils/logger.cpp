// 日志工具实现
#include "logger.h"
#include <iomanip>

// 静态实例初始化
Logger* Logger::instance = nullptr;

Logger::Logger() : currentLevel(LogLevel::INFO), consoleOutput(true) {
}

Logger::~Logger() {
    shutdown();
}

std::string Logger::levelToString(LogLevel level) {
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

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

Logger* Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

bool Logger::initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // 如果已经打开了日志文件，先关闭
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // 打开日志文件
    logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return false;
    }
    
    // 记录日志初始化信息
    log(LogLevel::INFO, "Logger initialized");
    return true;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

void Logger::setConsoleOutput(bool output) {
    std::lock_guard<std::mutex> lock(logMutex);
    consoleOutput = output;
}

void Logger::log(LogLevel level, const std::string& message) {
    // 检查日志级别
    if (level < currentLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string logMessage = "[" + getCurrentTime() + "] [" + levelToString(level) + "] " + message;
    
    // 输出到文件
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
        logFile.flush();  // 确保日志立即写入
    }
    
    // 输出到控制台
    if (consoleOutput) {
        // 根据日志级别设置不同的颜色
        switch (level) {
            case LogLevel::DEBUG:
                std::cout << "\033[37m" << logMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::INFO:
                std::cout << "\033[32m" << logMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::WARNING:
                std::cout << "\033[33m" << logMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::ERROR:
                std::cout << "\033[31m" << logMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::CRITICAL:
                std::cout << "\033[35m" << logMessage << "\033[0m" << std::endl;
                break;
        }
    }
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

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile << "[" << getCurrentTime() << "] [INFO] Logger shutdown" << std::endl;
        logFile.close();
    }
}
