#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "external/json.hpp"
#include "utils/logger.h"

namespace utils {

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool loadConfig(const std::string& configPath) {
        try {
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                LOG_ERROR("Failed to open config file: " + configPath);
                return false;
            }

            configFile >> config_;
            configLoaded_ = true;
            LOG_INFO("Config loaded successfully from: " + configPath);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Error loading config: " + std::string(e.what()));
            return false;
        }
    }

    // HTTP配置相关
    int getHttpPort() const {
        return getValue<int>("http.port", 8080);
    }

    std::string getHttpHost() const {
        return getValue<std::string>("http.host", "0.0.0.0");
    }

    // 数据库配置相关
    std::string getDatabasePath() const {
        return getValue<std::string>("database.path", ":memory:");
    }

    int getDatabaseMaxConnections() const {
        return getValue<int>("database.max_connections", 10);
    }

    // 缓存配置相关
    int getCacheCapacity() const {
        return getValue<int>("cache.capacity", 1000);
    }

    int getCacheTTLSeconds() const {
        return getValue<int>("cache.ttl_seconds", 3600);
    }

    // 短链接配置相关
    int getDefaultExpireSeconds() const {
        return getValue<int>("link.default_expire_seconds", 86400);
    }

    int getShortCodeLength() const {
        return getValue<int>("link.short_code_length", 6);
    }

    int getMaxCustomAliasLength() const {
        return getValue<int>("link.max_custom_alias_length", 20);
    }

    // 日志配置相关
    std::string getLogLevel() const {
        return getValue<std::string>("log.level", "INFO");
    }

    std::string getLogFile() const {
        return getValue<std::string>("log.file", "shortlink.log");
    }

    bool isConfigLoaded() const {
        return configLoaded_;
    }

private:
    ConfigManager() : configLoaded_(false) {}

    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const {
        if (!configLoaded_) {
            LOG_ERROR("Config not loaded yet");
            return defaultValue;
        }

        try {
            // 处理嵌套键，如 "http.port"
            const auto& keys = splitKey(key);
            const nlohmann::json* current = &config_;
            
            for (const auto& k : keys) {
                if (current->contains(k)) {
                    current = &((*current)[k]);
                } else {
                    return defaultValue;
                }
            }

            return current->get<T>();
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting config value for key " + key + ": " + std::string(e.what()));
            return defaultValue;
        }
    }

    std::vector<std::string> splitKey(const std::string& key) const {
        std::vector<std::string> keys;
        std::stringstream ss(key);
        std::string token;
        
        while (std::getline(ss, token, '.')) {
            if (!token.empty()) {
                keys.push_back(token);
            }
        }
        
        return keys;
    }

private:
    nlohmann::json config_;
    bool configLoaded_;
};

} // namespace utils
