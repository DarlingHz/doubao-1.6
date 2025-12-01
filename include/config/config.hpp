#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <optional>

namespace api_quota {
namespace config {

struct ServerConfig {
    int port = 8080;
    int thread_pool_size = 8;
    std::string host = "127.0.0.1";
    int request_timeout_seconds = 30;
};

struct DatabaseConfig {
    std::string sqlite_db_path = ":memory:";
    int max_connections = 10;
    int connection_timeout_seconds = 5;
    bool enable_wal = true;
};

struct CacheConfig {
    int client_cache_ttl_seconds = 300;    // 5分钟
    int api_key_cache_ttl_seconds = 600;   // 10分钟
    int quota_flush_interval_seconds = 60; // 1分钟
    int cache_capacity = 10000;
};

struct QuotaConfig {
    int default_daily_quota = 10000;
    int default_per_minute_quota = 200;
    int min_weight = 1;
    int max_weight = 100;
};

struct LoggingConfig {
    bool enable_console_logging = true;
    std::string log_level = "info";
    std::optional<std::string> log_file_path;
};

class ConfigManager {
public:
    static ConfigManager& get_instance();
    
    // 从文件加载配置
    bool load_from_file(const std::string& file_path);
    
    // 从环境变量加载配置
    bool load_from_env();
    
    // 设置默认配置
    void set_defaults();
    
    // 获取配置
    const ServerConfig& get_server_config() const;
    const DatabaseConfig& get_database_config() const;
    const CacheConfig& get_cache_config() const;
    const QuotaConfig& get_quota_config() const;
    const LoggingConfig& get_logging_config() const;
    
    // 修改配置（用于测试或运行时调整）
    void set_server_port(int port);
    void set_database_path(const std::string& path);
    void set_thread_pool_size(int size);
    
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    // 禁止拷贝和赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    ServerConfig server_config_;
    DatabaseConfig database_config_;
    CacheConfig cache_config_;
    QuotaConfig quota_config_;
    LoggingConfig logging_config_;
    
    // 从JSON字符串加载配置
    bool load_from_json(const std::string& json_str);
};

// 便捷函数
extern int get_server_port();
extern const std::string& get_database_path();
extern int get_thread_pool_size();
extern int get_default_daily_quota();
extern int get_default_per_minute_quota();

} // namespace config
} // namespace api_quota

#endif // CONFIG_HPP