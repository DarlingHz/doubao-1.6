// 服务器实现
#include "server.h"
#include "../utils/logger.h"
#include "../utils/stats_monitor.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <chrono>

Server::Server(int port, const std::string& host, int threadPoolSize)
    : port(port), host(host), threadPoolSize(threadPoolSize), running(false), serverSocket(-1) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    try {
        // 初始化日志
        Logger::getInstance()->info("Starting server on " + host + ":" + std::to_string(port));
        
        // 初始化路由
        router.initialize();
        
        // 初始化套接字
        initializeSocket();
        
        // 设置服务器状态为运行
        running = true;
        
        // 创建工作线程池
        for (int i = 0; i < threadPoolSize; ++i) {
            workerThreads.emplace_back(&Server::acceptConnections, this);
        }
        
        Logger::getInstance()->info("Server started successfully with " + 
                                   std::to_string(threadPoolSize) + " worker threads");
        Logger::getInstance()->info("Server info: " + getServerInfo());
        
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Failed to start server: " + std::string(e.what()));
        shutdownServer();
        return false;
    }
}

void Server::stop() {
    if (!running) {
        return;
    }
    
    Logger::getInstance()->info("Stopping server...");
    
    // 设置运行状态为停止
    running = false;
    
    // 关闭服务器套接字以唤醒阻塞的accept调用
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    // 等待所有工作线程结束
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    workerThreads.clear();
    
    // 记录服务器停止信息
    Logger::getInstance()->info("Server stopped");
}

bool Server::isRunning() const {
    return running;
}

std::string Server::getServerInfo() const {
    std::stringstream ss;
    ss << "Ride-sharing Matching System API Server" << ", "
       << "Port: " << port << ", "
       << "Host: " << host << ", "
       << "Thread Pool Size: " << threadPoolSize;
    return ss.str();
}

void Server::initializeSocket() {
    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // 设置套接字选项，允许重用地址
    int reuseAddr = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to set socket options");
    }
    
    // 绑定地址和端口
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to bind socket to " + host + ":" + std::to_string(port));
    }
    
    // 开始监听连接
    if (listen(serverSocket, 100) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    Logger::getInstance()->info("Socket initialized successfully");
}

void Server::acceptConnections() {
    while (running) {
        try {
            // 使用select进行非阻塞等待
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET(serverSocket, &readFds);
            
            struct timeval timeout;
            timeout.tv_sec = 1;  // 1秒超时，定期检查running标志
            timeout.tv_usec = 0;
            
            int ready = select(serverSocket + 1, &readFds, nullptr, nullptr, &timeout);
            
            if (ready < 0) {
                // 发生错误，检查是否是服务器停止导致的
                if (!running) {
                    break;
                }
                Logger::getInstance()->error("Select error in acceptConnections");
                continue;
            }
            
            if (ready == 0) {
                // 超时，继续循环检查running标志
                continue;
            }
            
            // 接受客户端连接
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            
            if (clientSocket < 0) {
                // 接受连接失败，检查是否是服务器停止导致的
                if (!running) {
                    break;
                }
                Logger::getInstance()->error("Failed to accept client connection");
                continue;
            }
            
            // 记录新的客户端连接
            std::string clientIp = inet_ntoa(clientAddr.sin_addr);
            Logger::getInstance()->debug("New connection from " + clientIp + ":" + 
                                       std::to_string(ntohs(clientAddr.sin_port)));
            
            // 处理客户端请求
            handleClient(clientSocket);
            
            // 关闭客户端套接字
            close(clientSocket);
            Logger::getInstance()->debug("Connection closed for " + clientIp);
            
        } catch (const std::exception& e) {
            // 捕获所有异常，防止工作线程意外退出
            if (running) {  // 只有在服务器运行时记录错误
                Logger::getInstance()->error("Exception in acceptConnections thread: " + std::string(e.what()));
            }
        }
    }
    
    Logger::getInstance()->info("Accept thread exiting");
}

void Server::handleClient(int clientSocket) {
    // 设置接收超时
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5秒超时
    timeout.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 接收请求数据
    std::string requestData;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        requestData += buffer;
        
        // 检查是否接收到完整的HTTP请求
        if (requestData.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    
    if (bytesRead < 0) {
        Logger::getInstance()->error("Failed to receive request data");
        return;
    }
    
    if (requestData.empty()) {
        Logger::getInstance()->warning("Empty request received");
        return;
    }
    
    try {
        // 解析HTTP请求
        HttpRequest request = parseRequest(requestData);
        
        // 使用路由器处理请求
        HttpResponse response = router.handleRequest(request);
        
        // 序列化响应
        std::string responseData = serializeResponse(response);
        
        // 发送响应
        send(clientSocket, responseData.c_str(), responseData.size(), 0);
        
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error handling client request: " + std::string(e.what()));
        
        // 发送错误响应
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n" 
                                  "Content-Type: application/json\r\n" 
                                  "Content-Length: 30\r\n" 
                                  "\r\n" 
                                  "{\"error\":\"Internal Server Error\"}";
        send(clientSocket, errorResponse.c_str(), errorResponse.size(), 0);
    }
}

HttpRequest Server::parseRequest(const std::string& requestData) {
    HttpRequest request;
    
    std::istringstream requestStream(requestData);
    std::string line;
    
    // 解析请求行
    if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path >> request.httpVersion;
    }
    
    // 解析请求头
    while (std::getline(requestStream, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // 去除前导空格和尾部的\r

            // 去除前导空格
            size_t startPos = value.find_first_not_of(" ");
            if (startPos != std::string::npos) {
                value = value.substr(startPos);
            }
            
            // 去除尾部的\r

            // 去除尾部的\r

            // 去除尾部的换行符
            size_t endPos = value.find_last_not_of("\r\n");
            if (endPos != std::string::npos) {
                value = value.substr(0, endPos + 1);
            }
            
            request.headers[key] = value;
        }
    }
    
    // 解析查询参数
    size_t queryPos = request.path.find('?');
    if (queryPos != std::string::npos) {
        std::string pathPart = request.path.substr(0, queryPos);
        std::string queryPart = request.path.substr(queryPos + 1);
        
        request.path = pathPart;
        
        // 解析查询参数
        size_t paramStart = 0;
        while (paramStart < queryPart.size()) {
            size_t paramEnd = queryPart.find('&', paramStart);
            if (paramEnd == std::string::npos) {
                paramEnd = queryPart.size();
            }
            
            std::string param = queryPart.substr(paramStart, paramEnd - paramStart);
            size_t equalsPos = param.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = param.substr(0, equalsPos);
                std::string value = param.substr(equalsPos + 1);
                request.queryParams[key] = value;
            }
            
            paramStart = paramEnd + 1;
        }
    }
    
    // 解析请求体
    // 检查Content-Length头
    auto contentLengthIt = request.headers.find("Content-Length");
    if (contentLengthIt != request.headers.end()) {
        try {
            size_t contentLength = std::stoul(contentLengthIt->second);
            
            // 跳过之前已经读取的部分
            while (std::getline(requestStream, line) && line != "\r") {
                // 跳过剩余的头部行
            }
            
            // 读取请求体
            if (contentLength > 0) {
                std::string body;
                body.resize(contentLength);
                size_t bytesRead = requestStream.read(&body[0], contentLength).gcount();
                body.resize(bytesRead);
                request.body = body;
            }
        } catch (...) {
            // Content-Length解析失败，忽略请求体
        }
    }
    
    return request;
}

std::string Server::serializeResponse(const HttpResponse& response) {
    std::stringstream responseStream;
    
    // 响应行
    responseStream << "HTTP/1.1 " << response.statusCode << " ";
    
    // 状态码文本
    switch (response.statusCode) {
        case 200: responseStream << "OK";
            break;
        case 201: responseStream << "Created";
            break;
        case 400: responseStream << "Bad Request";
            break;
        case 404: responseStream << "Not Found";
            break;
        case 500: responseStream << "Internal Server Error";
            break;
        default: responseStream << "Unknown";
    }
    responseStream << "\r\n";
    
    // 响应头
    for (const auto& [key, value] : response.headers) {
        responseStream << key << ": " << value << "\r\n";
    }
    
    // 添加Content-Length头（如果没有提供）
    if (response.headers.find("Content-Length") == response.headers.end()) {
        responseStream << "Content-Length: " << response.body.size() << "\r\n";
    }
    
    // 添加Connection头
    if (response.headers.find("Connection") == response.headers.end()) {
        responseStream << "Connection: close\r\n";
    }
    
    // 空行分隔头部和正文
    responseStream << "\r\n";
    
    // 响应体
    responseStream << response.body;
    
    return responseStream.str();
}

void Server::shutdownServer() {
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    running = false;
    
    // 等待工作线程结束
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
}
