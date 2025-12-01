// 服务器类 - 负责启动HTTP服务，处理客户端连接和请求分发
#ifndef SERVER_H
#define SERVER_H

#include "../controller/router.h"
#include <string>
#include <atomic>
#include <thread>
#include <vector>

class Server {
private:
    // 服务器配置
    int port;
    std::string host;
    int threadPoolSize;
    
    // 服务器状态
    std::atomic<bool> running;
    int serverSocket;
    
    // 路由管理器
    Router router;
    
    // 工作线程池
    std::vector<std::thread> workerThreads;
    
    // 私有方法
    void initializeSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    HttpRequest parseRequest(const std::string& requestData);
    std::string serializeResponse(const HttpResponse& response);
    void shutdownServer();
    
public:
    Server(int port = 8000, const std::string& host = "0.0.0.0", int threadPoolSize = 4);
    ~Server();
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 检查服务器是否正在运行
    bool isRunning() const;
    
    // 获取服务器信息
    std::string getServerInfo() const;
};

#endif // SERVER_H
