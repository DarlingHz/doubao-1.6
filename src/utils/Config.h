#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <memory>

namespace utils {

class Config {
public:
    static Config& getInstance();

    bool loadFromFile(const std::string& filePath);
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;

private:
    Config();
    ~Config();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::unordered_map<std::string, std::string> configData;
};

} // namespace utils

#endif // CONFIG_H