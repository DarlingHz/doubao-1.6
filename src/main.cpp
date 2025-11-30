#include "server.h"
#include "database.h"
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <chrono>
#include <thread>

// 全局服务器实例
Server* g_server = nullptr;

// 信号处理函数
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

// 执行 SQL 脚本初始化数据库
bool initializeDatabase(const std::string& dbPath, const std::string& schemaPath) {
    // 读取 SQL 脚本
    std::ifstream file(schemaPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open schema file: " << schemaPath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();
    
    // 连接数据库
    auto& db = Database::getInstance();
    if (!db.connect(dbPath)) {
        std::cerr << "Failed to connect to database: " << db.getErrorMessage() << std::endl;
        return false;
    }
    
    // 执行建表语句
    if (!db.execute(sql)) {
        std::cerr << "Failed to execute schema: " << db.getErrorMessage() << std::endl;
        return false;
    }
    
    std::cout << "Database initialized successfully" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    // 默认配置
    int port = 8080;
    std::string dbPath = "./inventory.db";
    std::string schemaPath = "./sql/schema.sql";
    
    // 解析命令行参数（简单解析）
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
        } else if (arg == "--db" && i + 1 < argc) {
            dbPath = argv[i + 1];
            i++;
        } else if (arg == "--schema" && i + 1 < argc) {
            schemaPath = argv[i + 1];
            i++;
        }
    }
    
    std::cout << "Starting Inventory Order Management System..." << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Database: " << dbPath << std::endl;
    std::cout << "Schema: " << schemaPath << std::endl;
    
    // 初始化数据库
    if (!initializeDatabase(dbPath, schemaPath)) {
        std::cerr << "Failed to initialize database. Exiting..." << std::endl;
        return 1;
    }
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 创建并启动服务器
    Server server(port);
    g_server = &server;
    
    if (!server.start()) {
        std::cerr << "Failed to start server. Exiting..." << std::endl;
        return 1;
    }
    
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    
    // 主循环 - 保持程序运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
