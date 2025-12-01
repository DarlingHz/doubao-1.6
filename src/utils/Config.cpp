#include "utils/Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

namespace utils {

Config::Config() {
    // 初始化一些默认配置值
    configData["http.port"] = "8080";
    configData["http.thread_pool_size"] = "4";
    configData["database.host"] = "localhost";
    configData["database.port"] = "3306";
    configData["log.level"] = "info";
}

Config::~Config() {
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filePath << std::endl;
            return false;
        }
        
        std::string line;
        std::regex keyValueRegex(R"((["'])([^"]+)\1:\s*["']?([^"',\s]+)["']?)");
        
        while (std::getline(file, line)) {
            std::smatch match;
            if (std::regex_search(line, match, keyValueRegex) && match.size() >= 4) {
                std::string key = match[2].str();
                std::string value = match[3].str();
                configData[key] = value;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return false;
    }
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = configData.find(key);
    if (it != configData.end()) {
        return it->second;
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    auto it = configData.find(key);
    if (it != configData.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    auto it = configData.find(key);
    if (it != configData.end()) {
        std::string value = it->second;
        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        }
        if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    auto it = configData.find(key);
    if (it != configData.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

} // namespace utils