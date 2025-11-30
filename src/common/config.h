#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
public:
    Config() = default;
    ~Config() = default;

    // 禁止拷贝和移动
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    // 单例模式
    static Config& Instance();

    // 加载配置文件
    bool Load(const std::string& file_path);

    // 获取字符串配置
    std::string GetString(const std::string& key, const std::string& default_val = "") const;

    // 获取整数配置
    int GetInt(const std::string& key, int default_val = 0) const;

    // 获取布尔配置
    bool GetBool(const std::string& key, bool default_val = false) const;

private:
    std::unordered_map<std::string, std::string> configs_;
};

#endif // CONFIG_H
