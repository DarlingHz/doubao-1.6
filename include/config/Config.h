#pragma once
#include <string>

namespace config {

struct Config {
    int http_port = 8080;
    std::string db_path = "./server_monitor.db";
    int alert_check_interval_sec = 60; // 告警检查间隔
    int db_connection_pool_size = 5;   // 数据库连接池大小
    int metrics_batch_size = 100;      // 指标批量写入大小
};

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    bool loadFromFile(const std::string& file_path);
    bool loadFromEnvironment();
    
    const Config& getConfig() const;
    
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    Config config_;
};

} // namespace config
