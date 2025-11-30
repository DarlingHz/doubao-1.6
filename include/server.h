#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// 简单的 HTTP 服务器类
class Server {
public:
    Server(int port);
    ~Server();
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
private:
    int port_;                  // 服务器端口
    int serverSocket_;          // 服务器 socket
    std::atomic<bool> running_; // 运行状态
    std::thread serverThread_;  // 服务器线程
    
    // 服务器主循环
    void run();
    
    // 处理客户端连接
    void handleClient(int clientSocket);
};

#endif // SERVER_H
