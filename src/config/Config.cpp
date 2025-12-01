#include <config/Config.h>
#include <util/Logger.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace config {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    // 默认配置
    config_.http_port = 8080;
    config_.db_path = "./server_monitor.db";
    config_.alert_check_interval_sec = 60;
    config_.db_connection_pool_size = 5;
    config_.metrics_batch_size = 100;
}

bool ConfigManager::loadFromFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_WARNING("配置文件不存在: " + file_path + ", 使用默认配置");
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (key == "HTTP_PORT") {
                config_.http_port = std::stoi(value);
            } else if (key == "DB_PATH") {
                config_.db_path = value;
            } else if (key == "ALERT_CHECK_INTERVAL_SEC") {
                config_.alert_check_interval_sec = std::stoi(value);
            } else if (key == "DB_CONNECTION_POOL_SIZE") {
                config_.db_connection_pool_size = std::stoi(value);
            } else if (key == "METRICS_BATCH_SIZE") {
                config_.metrics_batch_size = std::stoi(value);
            }
        }
    }
    
    LOG_INFO("配置文件加载成功: " + file_path);
    return true;
}

bool ConfigManager::loadFromEnvironment() {
    char* env = nullptr;
    
    env = std::getenv("HTTP_PORT");
    if (env) {
        config_.http_port = std::stoi(env);
    }
    
    env = std::getenv("DB_PATH");
    if (env) {
        config_.db_path = env;
    }
    
    env = std::getenv("ALERT_CHECK_INTERVAL_SEC");
    if (env) {
        config_.alert_check_interval_sec = std::stoi(env);
    }
    
    env = std::getenv("DB_CONNECTION_POOL_SIZE");
    if (env) {
        config_.db_connection_pool_size = std::stoi(env);
    }
    
    env = std::getenv("METRICS_BATCH_SIZE");
    if (env) {
        config_.metrics_batch_size = std::stoi(env);
    }
    
    LOG_INFO("环境变量配置加载成功");
    return true;
}

const Config& ConfigManager::getConfig() const {
    return config_;
}

} // namespace config
