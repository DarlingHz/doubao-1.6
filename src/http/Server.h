#ifndef SERVER_H
#define SERVER_H

#include "http/RequestHandler.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include <memory>
#include <atomic>
#include <thread>

namespace http {

class Server {
public:
    Server(std::shared_ptr<utils::Config> config,
          std::shared_ptr<utils::Logger> logger,
          std::shared_ptr<RequestHandler> request_handler);
    ~Server();
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 检查服务器是否运行
    bool isRunning() const { return running_; }
    
private:
    std::shared_ptr<utils::Config> config_;
    std::shared_ptr<utils::Logger> logger_;
    std::shared_ptr<RequestHandler> request_handler_;
    
    std::atomic<bool> running_;  // 服务器运行状态
    int server_fd_;             // 服务器文件描述符
    
    // 线程池相关
    std::vector<std::thread> thread_pool_;
    
    // 处理连接的工作线程函数
    void workerThread();
    
    // 监听端口
    bool bindAndListen(int port);
    
    // 处理客户端连接
    void handleConnection(int client_fd);
};

} // namespace http

#endif // SERVER_H
