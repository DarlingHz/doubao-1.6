#include "database.h"
#include "auth.h"
#include "http_server.h"
#include "user_controller.h"
#include "project_controller.h"
#include "task_controller.h"
#include "stats_controller.h"
#include <iostream>
#include <csignal>
#include <thread>

// 全局服务器实例指针，用于信号处理
HttpServer* g_server = nullptr;

// 信号处理函数，用于优雅地关闭服务器
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down server..." << std::endl;
    if (g_server != nullptr) {
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    try {
        // 配置数据库连接池
        std::string db_path = "team_management.db";
        DatabaseConnectionPool pool(db_path, 5); // 创建5个连接的连接池
        
        // 创建数据库访问实例
        auto db = std::make_shared<Database>(db_path);
        
        // 创建认证服务实例
        auto auth = std::make_shared<AuthService>(db);
        
        // 创建HTTP服务器实例，监听8080端口
        HttpServer server(8080, db, auth);
        g_server = &server;
        
        // 注册信号处理
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        // 创建并注册控制器
        UserController user_controller(db, auth);
        ProjectController project_controller(db, auth);
        TaskController task_controller(db, auth);
        StatsController stats_controller(db, auth);
        
        // 注册路由
        user_controller.registerRoutes(server);
        project_controller.registerRoutes(server);
        task_controller.registerRoutes(server);
        stats_controller.registerRoutes(server);
        
        std::cout << "Server starting on port 8080..." << std::endl;
        std::cout << "API endpoints available at: http://localhost:8080/api/v1/..." << std::endl;
        std::cout << "Press Ctrl+C to stop server" << std::endl;
        
        // 启动服务器
        server.start();
        
        // 保持主线程运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}