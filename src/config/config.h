// 配置管理类
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {
private:
    static Config* instance;
    
    // 数据库配置
    std::string dbPath;
    
    // 服务器配置
    int serverPort;
    int threadPoolSize;
    
    // 匹配配置
    int maxMatchingDistance;  // 最大匹配距离（曼哈顿距离）
    
    Config();
    
public:
    static Config* getInstance();
    
    // 获取配置项
    const std::string& getDbPath() const { return dbPath; }
    int getServerPort() const { return serverPort; }
    int getThreadPoolSize() const { return threadPoolSize; }
    int getMaxMatchingDistance() const { return maxMatchingDistance; }
    
    // 设置配置项（可用于运行时调整）
    void setDbPath(const std::string& path) { dbPath = path; }
    void setServerPort(int port) { serverPort = port; }
    void setThreadPoolSize(int size) { threadPoolSize = size; }
    void setMaxMatchingDistance(int distance) { maxMatchingDistance = distance; }
    
    // 加载配置（可从文件或环境变量）
    void loadFromFile(const std::string& filename);
    void loadFromEnvironment();
};

#endif // CONFIG_H
