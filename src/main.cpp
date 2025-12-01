// 主程序入口
#include "utils/logger.h"
#include "utils/stats_monitor.h"
#include "server/server.h"
#include <signal.h>
#include <iostream>
#include <chrono>
#include <thread>

// 全局服务器实例指针（用于信号处理）
Server* g_server = nullptr;

// 信号处理函数
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    
    if (g_server) {
        g_server->stop();
    }
    
    // 给服务器一些时间来优雅关闭
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 清理资源
    Logger::getInstance()->info("Server shutdown completed");
    Logger::getInstance()->close();
    
    exit(signum);
}

int main(int argc, char* argv[]) {
    try {
        // 注册信号处理函数
        signal(SIGINT, signalHandler);  // Ctrl+C
        signal(SIGTERM, signalHandler); // 终止信号
        
        // 初始化日志系统
        Logger::getInstance()->setLogLevel("INFO");
        Logger::getInstance()->setConsoleOutput(true);
        Logger::getInstance()->info("Starting Ride-sharing Matching System...");
        
        // 初始化统计监控系统
        StatsMonitor::getInstance()->initialize();
        Logger::getInstance()->info("Statistics monitoring initialized");
        
        // 创建服务器实例
        int port = 8000;
        int threadPoolSize = 8;
        
        // 从命令行参数读取端口号（如果提供）
        if (argc > 1) {
            try {
                port = std::stoi(argv[1]);
            } catch (...) {
                Logger::getInstance()->warning("Invalid port number, using default port 8000");
            }
        }
        
        // 从命令行参数读取线程池大小（如果提供）
        if (argc > 2) {
            try {
                threadPoolSize = std::stoi(argv[2]);
            } catch (...) {
                Logger::getInstance()->warning("Invalid thread pool size, using default size 8");
            }
        }
        
        g_server = new Server(port, "0.0.0.0", threadPoolSize);
        
        // 启动服务器
        if (!g_server->start()) {
            Logger::getInstance()->error("Failed to start server");
            delete g_server;
            g_server = nullptr;
            Logger::getInstance()->close();
            return 1;
        }
        
        // 显示启动信息
        std::cout << "\n=== Ride-sharing Matching System Started ===" << std::endl;
        std::cout << "Server is running on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << "============================================" << std::endl;
        
        // 主循环 - 保持程序运行
        while (g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 清理资源
        delete g_server;
        g_server = nullptr;
        
        Logger::getInstance()->info("Main program exited normally");
        Logger::getInstance()->close();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        
        // 清理资源
        if (g_server) {
            delete g_server;
            g_server = nullptr;
        }
        
        Logger::getInstance()->error("Fatal exception: " + std::string(e.what()));
        Logger::getInstance()->close();
        
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        
        // 清理资源
        if (g_server) {
            delete g_server;
            g_server = nullptr;
        }
        
        Logger::getInstance()->error("Unknown fatal exception");
        Logger::getInstance()->close();
        
        return 1;
    }
}
