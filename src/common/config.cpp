#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

Config& Config::Instance() {
    static Config instance;
    return instance;
}

bool Config::Load(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 移除注释
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        // 移除首尾空格
        auto trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
            s.erase(std::find_if(s.rbegin(), s.rend(),
                [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), s.end());
        };
        trim(line);

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 解析键值对
        size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, equal_pos);
        std::string value = line.substr(equal_pos + 1);
        trim(key);
        trim(value);

        if (!key.empty()) {
            configs_[key] = value;
        }
    }

    file.close();
    return true;
}

std::string Config::GetString(const std::string& key, const std::string& default_val) const {
    auto it = configs_.find(key);
    return it != configs_.end() ? it->second : default_val;
}

int Config::GetInt(const std::string& key, int default_val) const {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
        return default_val;
    }

    try {
        return std::stoi(it->second);
    } catch (...) {
        return default_val;
    }
}

bool Config::GetBool(const std::string& key, bool default_val) const {
    auto it = configs_.find(key);
    if (it == configs_.end()) {
        return default_val;
    }

    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    if (value == "true" || value == "1" || value == "yes") {
        return true;
    } else if (value == "false" || value == "0" || value == "no") {
        return false;
    } else {
        return default_val;
    }
}
