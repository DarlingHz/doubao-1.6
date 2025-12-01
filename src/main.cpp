#include "include/controller/ApiController.h"
#include "include/db/ConnectionPool.h"
#include "include/db/MySQLConnectionPool.h"
#include "include/db/DatabaseConfig.h"
#include <httplib.h>
#include <iostream>
#include <memory>

// 配置数据库连接
void initDatabase(const db::DatabaseConfig& config) {
    auto pool = db::MySQLConnectionPool::getInstance();
    pool->init(config);
}

// 注册API路由
void registerRoutes(httplib::Server& server, controller::ApiController& controller) {
    // 公开路由（不需要认证）
    server.Post("/api/register", [&controller](const httplib::Request& req, httplib::Response& res) {
        res.set_content(controller.handleRegister(req.body), "application/json");
    });

    server.Post("/api/login", [&controller](const httplib::Request& req, httplib::Response& res) {
        res.set_content(controller.handleLogin(req.body), "application/json");
    });

    // 需要认证的路由
    auto auth_required = [&controller](const httplib::Request& req, httplib::Response& res) -> bool {
        auto it = req.headers.find("Authorization");
        if (it == req.headers.end()) {
            res.set_content(controller.buildErrorResponse(401, "未提供认证信息"), "application/json");
            return false;
        }

        int userId = controller.validateAuthToken(it->second);
        if (userId <= 0) {
            res.set_content(controller.buildErrorResponse(401, "无效的认证信息"), "application/json");
            return false;
        }

        // 将userId存储在请求上下文中，供处理函数使用
        req.user_data = std::make_shared<int>(userId);
        return true;
    };

    // 卡片相关路由
    server.Post("/api/cards", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleCreateCard(req.body, userId), "application/json");
    });

    server.Get("/api/cards", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleGetCardList(req.query_string, userId), "application/json");
    });

    server.Get(R"(^/api/cards/(\d+)$)", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        std::string cardId = req.matches[1];
        res.set_content(controller.handleGetCard(cardId, userId), "application/json");
    });

    server.Put(R"(^/api/cards/(\d+)$)", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        std::string cardId = req.matches[1];
        res.set_content(controller.handleUpdateCard(cardId, req.body, userId), "application/json");
    });

    server.Delete(R"(^/api/cards/(\d+)$)", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        std::string cardId = req.matches[1];
        res.set_content(controller.handleDeleteCard(cardId, userId), "application/json");
    });

    // 卡片搜索路由
    server.Get("/api/cards/search", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleSearchCards(req.query_string, userId), "application/json");
    });

    // 标签相关路由
    server.Get("/api/tags", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleGetTags(userId), "application/json");
    });

    server.Post("/api/tags/rename", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleRenameTag(req.body, userId), "application/json");
    });

    server.Post("/api/tags/merge", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleMergeTags(req.body, userId), "application/json");
    });

    server.Get("/api/tags/top", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        std::string limit = req.get_param_value("limit");
        res.set_content(controller.handleGetTopTags(userId, limit), "application/json");
    });

    // 统计路由
    server.Get("/api/statistics", auth_required, [&controller](const httplib::Request& req, httplib::Response& res) {
        int userId = *static_cast<int*>(req.user_data.get());
        res.set_content(controller.handleGetStatistics(userId), "application/json");
    });

    // 健康检查路由
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });
}

int main(int argc, char* argv[]) {
    // 数据库配置
    db::DatabaseConfig config;
    config.host = "localhost";
    config.port = 3306;
    config.database = "knowledge_cards";
    config.username = "root";
    config.password = "password";  // 在生产环境中应该使用环境变量或配置文件
    config.poolSize = 10;

    try {
        // 初始化数据库连接池
        initDatabase(config);
        std::cout << "数据库连接池初始化成功" << std::endl;

        // 创建控制器
        controller::ApiController apiController;

        // 创建HTTP服务器
        httplib::Server server;

        // 注册路由
        registerRoutes(server, apiController);

        // 添加CORS中间件
        server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            
            if (req.method == "OPTIONS") {
                res.status = 200;
                return httplib::Server::HandlerResponse::Handled;
            }
            
            return httplib::Server::HandlerResponse::Unhandled;
        });

        // 设置最大请求体大小
        server.set_payload_max_length(1024 * 1024 * 10);  // 10MB

        // 启动服务器
        int port = 8080;
        std::cout << "服务器启动在端口 " << port << std::endl;
        std::cout << "健康检查: http://localhost:" << port << "/health" << std::endl;
        
        server.listen("0.0.0.0", port);
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}