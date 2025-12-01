#include "config/config.hpp"
#include "utils/json.hpp"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace api_quota {
namespace config {

ConfigManager::ConfigManager() {
    set_defaults();
    // 尝试从环境变量加载配置
    load_from_env();
}

ConfigManager& ConfigManager::get_instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::set_defaults() {
    // ServerConfig defaults are set in the struct definition
    // DatabaseConfig defaults are set in the struct definition
    // CacheConfig defaults are set in the struct definition
    // QuotaConfig defaults are set in the struct definition
    // LoggingConfig defaults are set in the struct definition
    
    // 可以在这里添加额外的默认设置
    server_config_.port = 8080;
    server_config_.host = "127.0.0.1";
    server_config_.thread_pool_size = 8;
    
    database_config_.sqlite_db_path = "./api_quota.db";
    database_config_.max_connections = 10;
    
    cache_config_.client_cache_ttl_seconds = 300;
    cache_config_.api_key_cache_ttl_seconds = 600;
    cache_config_.quota_flush_interval_seconds = 60;
    
    quota_config_.default_daily_quota = 10000;
    quota_config_.default_per_minute_quota = 200;
}

bool ConfigManager::load_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << file_path << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return load_from_json(buffer.str());
}

bool ConfigManager::load_from_env() {
    // 加载服务器配置
    if (const char* port = std::getenv("API_QUOTA_PORT")) {
        try {
            server_config_.port = std::stoi(port);
        } catch (...) {
            std::cerr << "Invalid API_QUOTA_PORT value" << std::endl;
        }
    }
    
    if (const char* host = std::getenv("API_QUOTA_HOST")) {
        server_config_.host = host;
    }
    
    if (const char* threads = std::getenv("API_QUOTA_THREADS")) {
        try {
            server_config_.thread_pool_size = std::stoi(threads);
        } catch (...) {
            std::cerr << "Invalid API_QUOTA_THREADS value" << std::endl;
        }
    }
    
    // 加载数据库配置
    if (const char* db_path = std::getenv("API_QUOTA_DB_PATH")) {
        database_config_.sqlite_db_path = db_path;
    }
    
    // 加载缓存配置
    if (const char* ttl = std::getenv("API_QUOTA_CLIENT_CACHE_TTL")) {
        try {
            cache_config_.client_cache_ttl_seconds = std::stoi(ttl);
        } catch (...) {
            std::cerr << "Invalid API_QUOTA_CLIENT_CACHE_TTL value" << std::endl;
        }
    }
    
    // 加载配额配置
    if (const char* daily = std::getenv("API_QUOTA_DEFAULT_DAILY_QUOTA")) {
        try {
            quota_config_.default_daily_quota = std::stoi(daily);
        } catch (...) {
            std::cerr << "Invalid API_QUOTA_DEFAULT_DAILY_QUOTA value" << std::endl;
        }
    }
    
    if (const char* minute = std::getenv("API_QUOTA_DEFAULT_PER_MINUTE_QUOTA")) {
        try {
            quota_config_.default_per_minute_quota = std::stoi(minute);
        } catch (...) {
            std::cerr << "Invalid API_QUOTA_DEFAULT_PER_MINUTE_QUOTA value" << std::endl;
        }
    }
    
    return true;
}

bool ConfigManager::load_from_json(const std::string& json_str) {
    auto json_opt = utils::parse_json(json_str);
    if (!json_opt) {
        std::cerr << "Failed to parse JSON config" << std::endl;
        return false;
    }
    
    const auto& json = *json_opt;
    
    // 解析服务器配置
    if (json.contains("server")) {
        const auto& server_json = json["server"];
        if (server_json.contains("port")) {
            server_config_.port = server_json["port"].as_integer();
        }
        if (server_json.contains("host")) {
            server_config_.host = server_json["host"].as_string();
        }
        if (server_json.contains("thread_pool_size")) {
            server_config_.thread_pool_size = server_json["thread_pool_size"].as_integer();
        }
    }
    
    // 解析数据库配置
    if (json.contains("database")) {
        const auto& db_json = json["database"];
        if (db_json.contains("sqlite_db_path")) {
            database_config_.sqlite_db_path = db_json["sqlite_db_path"].as_string();
        }
    }
    
    // 解析缓存配置
    if (json.contains("cache")) {
        const auto& cache_json = json["cache"];
        if (cache_json.contains("client_cache_ttl_seconds")) {
            cache_config_.client_cache_ttl_seconds = cache_json["client_cache_ttl_seconds"].as_integer();
        }
    }
    
    // 解析配额配置
    if (json.contains("quota")) {
        const auto& quota_json = json["quota"];
        if (quota_json.contains("default_daily_quota")) {
            quota_config_.default_daily_quota = quota_json["default_daily_quota"].as_integer();
        }
        if (quota_json.contains("default_per_minute_quota")) {
            quota_config_.default_per_minute_quota = quota_json["default_per_minute_quota"].as_integer();
        }
    }
    
    return true;
}

const ServerConfig& ConfigManager::get_server_config() const {
    return server_config_;
}

const DatabaseConfig& ConfigManager::get_database_config() const {
    return database_config_;
}

const CacheConfig& ConfigManager::get_cache_config() const {
    return cache_config_;
}

const QuotaConfig& ConfigManager::get_quota_config() const {
    return quota_config_;
}

const LoggingConfig& ConfigManager::get_logging_config() const {
    return logging_config_;
}

void ConfigManager::set_server_port(int port) {
    server_config_.port = port;
}

void ConfigManager::set_database_path(const std::string& path) {
    database_config_.sqlite_db_path = path;
}

void ConfigManager::set_thread_pool_size(int size) {
    server_config_.thread_pool_size = size;
}

// 便捷函数实现
int get_server_port() {
    return ConfigManager::get_instance().get_server_config().port;
}

const std::string& get_database_path() {
    return ConfigManager::get_instance().get_database_config().sqlite_db_path;
}

int get_thread_pool_size() {
    return ConfigManager::get_instance().get_server_config().thread_pool_size;
}

int get_default_daily_quota() {
    return ConfigManager::get_instance().get_quota_config().default_daily_quota;
}

int get_default_per_minute_quota() {
    return ConfigManager::get_instance().get_quota_config().default_per_minute_quota;
}

} // namespace config
} // namespace api_quota