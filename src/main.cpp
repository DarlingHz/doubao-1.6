#include "http/server.hpp"
#include "config/config.hpp"
#include "repository/client_repository.hpp"
#include "repository/api_key_repository.hpp"
#include "repository/log_repository.hpp"
#include "service/client_service.hpp"
#include "service/api_key_service.hpp"
#include "service/quota_service.hpp"
#include "utils/memory_cache.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    try {
        // 初始化配置
        auto& config = api_quota::config::ConfigManager::get_instance();
        config.load_from_env(); // 从环境变量加载配置
        
        // 为了简化实现，使用空的void指针作为缓存
        auto cache = std::shared_ptr<void>();
        
        // 初始化数据库仓库
        auto client_repo = std::make_shared<api_quota::repository::ClientRepository>(config.get_database_config().sqlite_db_path);
        auto api_key_repo = std::make_shared<api_quota::repository::ApiKeyRepository>(config.get_database_config().sqlite_db_path);
        auto log_repo = std::make_shared<api_quota::repository::LogRepository>(config.get_database_config().sqlite_db_path);
        
        // 初始化服务层
        auto client_service = std::make_shared<api_quota::service::ClientService>(client_repo);
        auto api_key_service = std::make_shared<api_quota::service::ApiKeyService>(api_key_repo, client_repo);
        auto quota_service = std::make_shared<api_quota::service::QuotaService>(api_key_service, api_key_repo, client_repo, log_repo);
        
        // 初始化HTTP服务器
        auto server = std::make_shared<api_quota::http::Server>(
            config.get_server_config().host,
            config.get_server_config().port,
            client_service,
            api_key_service,
            quota_service
        );
        
        // 启动清理线程，定期清理过期缓存和日志
        std::thread cleanup_thread([&log_repo]() {
            while (true) {
                try {
                    // 清理过期日志 - 使用默认值30天
                    log_repo->cleanup_old_logs(30);
                    
                } catch (const std::exception& e) {
                    std::cerr << "Cleanup error: " << e.what() << std::endl;
                }
                
                // 每隔5分钟执行一次清理
                std::this_thread::sleep_for(std::chrono::minutes(5));
            }
        });
        cleanup_thread.detach();
        
        // 启动HTTP服务器
        std::cout << "Starting API Quota Manager server on " 
                  << config.get_server_config().host << ":" 
                  << config.get_server_config().port << std::endl;
        
        server->start();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}