#include "server.h"
#include "request_handler.h"
#include <iostream>
#include <thread>
#include <sstream>
#include <string>
#include <cstring>

Server::Server(int port) : port_(port), running_(false) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    // 创建 socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // 设置 socket 选项
    int reuse = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // 监听
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    running_ = true;
    
    // 启动服务器线程
    serverThread_ = std::thread(&Server::run, this);
    
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 关闭服务器 socket
    close(serverSocket_);
    
    // 等待服务器线程结束
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void Server::run() {
    while (running_) {
        // 接受连接
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        // 使用 select 进行超时监听，避免阻塞
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket_, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1秒超时
        timeout.tv_usec = 0;
        
        int activity = select(serverSocket_ + 1, &readSet, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            continue;  // 出错，继续下一次循环
        }
        
        if (activity == 0) {
            continue;  // 超时，继续下一次循环
        }
        
        if (FD_ISSET(serverSocket_, &readSet)) {
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
            
            if (clientSocket < 0) {
                continue;
            }
            
            // 处理客户端请求
            handleClient(clientSocket);
        }
    }
}

void Server::handleClient(int clientSocket) {
    const int BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE] = {0};
    
    // 读取请求
    ssize_t bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
    
    if (bytesRead > 0) {
        std::string request(buffer, bytesRead);
        
        // 解析并处理请求
        RequestHandler handler;
        std::string response = handler.processRequest(request);
        
        // 发送响应
        send(clientSocket, response.c_str(), response.size(), 0);
    }
    
    close(clientSocket);
}
