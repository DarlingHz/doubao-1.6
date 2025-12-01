#include "http/Server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <iostream>
#include <sstream> // 添加istringstream的头文件

namespace http {

// 全局信号处理
static std::atomic<bool> g_shutdown = false;

static void signalHandler(int sig) {
    g_shutdown = true;
}

Server::Server(std::shared_ptr<utils::Config> config,
              std::shared_ptr<utils::Logger> logger,
              std::shared_ptr<RequestHandler> request_handler)
    : config_(config),
      logger_(logger),
      request_handler_(request_handler),
      running_(false),
      server_fd_(-1) {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
}

Server::~Server() {
    stop();
}

bool Server::start() {
    int port = config_->getInt("http.port", 8080);
    int thread_pool_size = config_->getInt("http.thread_pool_size", 4);
    
    // 绑定端口并开始监听
    if (!bindAndListen(port)) {
        std::stringstream ss;
        ss << "Failed to bind and listen on port " << port;
        logger_->error(ss.str());
        return false;
    }
    
    running_ = true;
    std::stringstream ss;
    ss << "Server started on port " << port;
    logger_->info(ss.str());
    
    // 创建线程池
    for (int i = 0; i < thread_pool_size; ++i) {
        thread_pool_.emplace_back(&Server::workerThread, this);
    }
    
    // 主线程等待信号
    while (!g_shutdown && running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 收到停止信号
    stop();
    return true;
}

void Server::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 关闭服务器套接字
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    
    // 等待所有工作线程结束
    for (auto& thread : thread_pool_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    thread_pool_.clear();
    
    logger_->info("Server stopped");
}

bool Server::bindAndListen(int port) {
    // 创建套接字
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        logger_->error("Failed to create socket");
        return false;
    }
    
    // 设置SO_REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_->error("Failed to set socket options");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::stringstream ss;
        ss << "Failed to bind socket to port " << port;
        logger_->error(ss.str());
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // 开始监听
    if (listen(server_fd_, 100) < 0) {
        logger_->error("Failed to listen on socket");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    return true;
}

void Server::workerThread() {
    while (running_) {
        // 使用非阻塞方式接受连接
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // 设置非阻塞
        int flags = fcntl(server_fd_, F_GETFL, 0);
        fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            // 处理连接
            handleConnection(client_fd);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::stringstream ss;
            ss << "Accept error: " << strerror(errno);
            logger_->error(ss.str());
        }
        
        // 短暂休眠避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void Server::handleConnection(int client_fd) {
    // 记录开始时间
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // 读取请求数据
        char buffer[1024 * 1024]; // 1MB 缓冲区
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            // 解析并处理请求
            auto response = request_handler_->handleRequest(request);
            
            // 发送响应
            write(client_fd, response.c_str(), response.length());
            
            // 记录请求信息
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            // 简单解析请求方法和路径
            std::string method, path;
            std::istringstream iss(request);
            iss >> method >> path;
            
            std::stringstream ss;
            ss << method << " " << path << " " << duration << "ms";
            logger_->info(ss.str());
        }
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "Error handling connection: " << e.what();
        logger_->error(ss.str());
        
        // 发送错误响应
        std::string error_response = "HTTP/1.1 500 Internal Server Error\r\n";
        error_response += "Content-Type: application/json\r\n";
        error_response += "\r\n";
        error_response += "{\"code\": 500, \"message\": \"Internal Server Error\", \"data\": null}";
        
        write(client_fd, error_response.c_str(), error_response.length());
    }
    
    // 关闭连接
    close(client_fd);
}

} // namespace http
