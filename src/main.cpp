#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "external/httplib.h"
#include "../include/http/controller.h"
#include "../include/service/link_service.h"
#include "../include/storage/database.h"
#include "../include/utils/config.h"
#include "../include/utils/logger.h"
#include "../include/utils/time_utils.h"

// 清理缓存的定时任务
void cacheCleanupTask(std::shared_ptr<service::LinkService> linkService, int intervalSeconds) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
        try {
            linkService->cleanupCache();
        } catch (const std::exception& e) {
            utils::LOG_ERROR("Error in cache cleanup task: " + std::string(e.what()));
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // 初始化配置管理器
        auto& config = utils::ConfigManager::getInstance();
        
        // 确定配置文件路径
        std::string configPath = "config/config.json";
        if (argc > 1) {
            configPath = argv[1];
        }
        
        // 加载配置
        if (!config.loadConfig(configPath)) {
            std::cerr << "Failed to load config file: " << configPath << std::endl;
            return 1;
        }
        
        // 初始化日志
        auto logLevelStr = config.getLogLevel();
        auto logFile = config.getLogFile();
        utils::LogLevel logLevel = (logLevelStr == "ERROR") ? utils::LogLevel::ERROR : utils::LogLevel::INFO;
        utils::Logger::getInstance().init(logFile, logLevel);
        
        utils::LOG_INFO("Starting ShortLink Service...");
        utils::LOG_INFO("Config loaded from: " + configPath);
        
        // 初始化数据库
        auto dbPath = config.getDatabasePath();
        auto database = std::make_shared<storage::Database>(dbPath);
        
        if (!database->initialize()) {
            utils::LOG_ERROR("Failed to initialize database");
            return 1;
        }
        
        // 初始化短链接服务
        auto linkService = std::make_shared<service::LinkService>(database, std::shared_ptr<utils::ConfigManager>(&config, [](auto*){}));
        
        if (!linkService->initialize()) {
            utils::LOG_ERROR("Failed to initialize link service");
            return 1;
        }
        
        // 启动缓存清理线程
        std::thread cacheThread(cacheCleanupTask, linkService, 300);  // 每5分钟清理一次
        cacheThread.detach();
        
        // 创建HTTP服务器
        httplib::Server server;
        
        // 配置服务器线程池
        server.set_mount_point("/", "./public");  // 如果需要静态文件服务
        server.set_read_timeout(5, 0);  // 5秒读超时
        server.set_write_timeout(5, 0);  // 5秒写超时
        server.set_idle_interval(30);  // 30秒空闲连接超时
        server.set_payload_max_length(1024 * 1024);  // 1MB请求体限制
        
        // 创建并注册控制器
        auto controller = std::make_shared<http::Controller>(linkService);
        controller->registerRoutes(server);
        
        // 获取HTTP配置
        int port = config.getHttpPort();
        std::string host = config.getHttpHost();
        
        utils::LOG_INFO("Server starting on " + host + ":" + std::to_string(port));
        utils::LOG_INFO("Health check: http://" + host + ":" + std::to_string(port) + "/health");
        utils::LOG_INFO("Create short link: POST http://" + host + ":" + std::to_string(port) + "/api/v1/shorten");
        utils::LOG_INFO("Redirect endpoint: GET http://" + host + ":" + std::to_string(port) + "/s/{short_code}");
        
        // 启动服务器
        if (!server.listen(host.c_str(), port)) {
            utils::LOG_ERROR("Failed to start server on " + host + ":" + std::to_string(port));
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        utils::LOG_ERROR("Unhandled exception: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}
