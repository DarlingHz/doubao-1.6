// 配置管理实现
#include "config.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

// 静态实例初始化
Config* Config::instance = nullptr;

Config::Config() {
    // 设置默认配置
    dbPath = "./carpool.db";
    serverPort = 8080;
    threadPoolSize = 4;
    maxMatchingDistance = 50;  // 默认最大匹配距离为50单位
    
    // 尝试从环境变量加载
    loadFromEnvironment();
}

Config* Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return instance;
}

void Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 设置配置项
            if (key == "DB_PATH") {
                dbPath = value;
            } else if (key == "SERVER_PORT") {
                serverPort = std::stoi(value);
            } else if (key == "THREAD_POOL_SIZE") {
                threadPoolSize = std::stoi(value);
            } else if (key == "MAX_MATCHING_DISTANCE") {
                maxMatchingDistance = std::stoi(value);
            }
        }
    }
    
    file.close();
}

void Config::loadFromEnvironment() {
    // 尝试从环境变量加载配置
    char* envValue = nullptr;
    
    envValue = std::getenv("CARPOOL_DB_PATH");
    if (envValue) {
        dbPath = envValue;
    }
    
    envValue = std::getenv("CARPOOL_SERVER_PORT");
    if (envValue) {
        serverPort = std::stoi(envValue);
    }
    
    envValue = std::getenv("CARPOOL_THREAD_POOL_SIZE");
    if (envValue) {
        threadPoolSize = std::stoi(envValue);
    }
    
    envValue = std::getenv("CARPOOL_MAX_MATCHING_DISTANCE");
    if (envValue) {
        maxMatchingDistance = std::stoi(envValue);
    }
}
