#include "HttpServer.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <algorithm>

HttpServer::HttpServer(int port) : port_(port), running_(false), serverSocket_(-1) {
    // 默认错误处理函数
    errorHandler_ = [](const HttpRequest& /*req*/, const std::exception& ex) {
        HttpResponse res(500);
        res.body = std::string("{\"error\":\"Internal Server Error\",\"message\":\"") + 
                  ex.what() + "\"}";
        return res;
    };
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) return;
    
    // 创建socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // 设置socket选项
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址和端口
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind socket");
    }
    
    // 开始监听
    if (listen(serverSocket_, 10) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    running_ = true;
    std::cout << "Server started on port " << port_ << "..." << std::endl;
    
    // 主线程接受连接
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        // 为每个客户端创建一个新线程处理请求
        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void HttpServer::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // 关闭所有客户端连接
    for (int clientSocket : clientSockets_) {
        close(clientSocket);
    }
    clientSockets_.clear();
    
    // 关闭服务器socket
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    std::cout << "Server stopped" << std::endl;
}

void HttpServer::get(const std::string& path, RequestHandler handler) {
    routes_.push_back({"GET", path, handler});
}

void HttpServer::post(const std::string& path, RequestHandler handler) {
    routes_.push_back({"POST", path, handler});
}

void HttpServer::put(const std::string& path, RequestHandler handler) {
    routes_.push_back({"PUT", path, handler});
}

void HttpServer::del(const std::string& path, RequestHandler handler) {
    routes_.push_back({"DELETE", path, handler});
}

void HttpServer::setErrorHandler(std::function<HttpResponse(const HttpRequest&, const std::exception&)> handler) {
    errorHandler_ = handler;
}

void HttpServer::handleClient(int clientSocket) {
    // 保存客户端socket到列表中
    { 
        clientSockets_.push_back(clientSocket);
    }
    
    try {
        // 读取请求数据
        char buffer[1024 * 10]; // 10KB缓冲区
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string requestStr(buffer);
            
            // 解析请求
            HttpRequest request = parseRequest(requestStr);
            
            // 处理OPTIONS请求
            if (request.method == "OPTIONS") {
                HttpResponse res(200);
                res.body = "";
                std::string responseStr = formatResponse(res);
                write(clientSocket, responseStr.c_str(), responseStr.size());
            } else {
                // 匹配路由
                RequestHandler handler = matchRoute(request.method, request.path, request.params);
                
                HttpResponse response;
                if (handler) {
                    // 调用处理函数
                    try {
                        response = handler(request);
                    } catch (const std::exception& ex) {
                        // 调用错误处理函数
                        response = errorHandler_(request, ex);
                    }
                } else {
                    // 404 Not Found
                    response.statusCode = 404;
                    response.body = "{\"error\":\"Not Found\"}";
                }
                
                // 发送响应
                std::string responseStr = formatResponse(response);
                write(clientSocket, responseStr.c_str(), responseStr.size());
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error handling client: " << ex.what() << std::endl;
    }
    
    // 关闭连接并从列表中移除
    close(clientSocket);
    auto it = std::find(clientSockets_.begin(), clientSockets_.end(), clientSocket);
    if (it != clientSockets_.end()) {
        clientSockets_.erase(it);
    }
}

HttpRequest HttpServer::parseRequest(const std::string& requestStr) {
    HttpRequest request;
    std::istringstream stream(requestStr);
    std::string line;
    
    // 解析请求行
    std::getline(stream, line);
    std::istringstream requestLine(line);
    requestLine >> request.method >> request.path;
    
    // 解析路径参数
    size_t queryPos = request.path.find('?');
    if (queryPos != std::string::npos) {
        std::string queryString = request.path.substr(queryPos + 1);
        request.path = request.path.substr(0, queryPos);
        
        // 解析查询参数
        size_t pos = 0;
        while (pos < queryString.size()) {
            size_t endPos = queryString.find('&', pos);
            if (endPos == std::string::npos) {
                endPos = queryString.size();
            }
            
            std::string param = queryString.substr(pos, endPos - pos);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                request.params[key] = value;
            }
            
            pos = endPos + 1;
        }
    }
    
    // 解析头部
    while (std::getline(stream, line) && line != "\r") {
        if (line.empty()) break;
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2); // 跳过": "
            // 移除可能的\r

            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            request.headers[key] = value;
        }
    }
    
    // 解析请求体
    std::string body;
    while (std::getline(stream, line)) {
        body += line;
    }
    request.body = body;
    
    return request;
}

std::string HttpServer::formatResponse(const HttpResponse& response) {
    std::stringstream ss;
    
    // 状态行
    ss << "HTTP/1.1 " << response.statusCode << " ";
    switch (response.statusCode) {
        case 200: ss << "OK";
            break;
        case 201: ss << "Created";
            break;
        case 204: ss << "No Content";
            break;
        case 400: ss << "Bad Request";
            break;
        case 404: ss << "Not Found";
            break;
        case 500: ss << "Internal Server Error";
            break;
        default: ss << "Unknown";
    }
    ss << "\r\n";
    
    // 头部
    for (const auto& [key, value] : response.headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    // 内容长度
    ss << "Content-Length: " << response.body.size() << "\r\n";
    
    // 空行分隔头部和正文
    ss << "\r\n";
    
    // 正文
    ss << response.body;
    
    return ss.str();
}

RequestHandler HttpServer::matchRoute(const std::string& method, const std::string& path, std::unordered_map<std::string, std::string>& /*params*/) {
    for (const auto& route : routes_) {
        if (route.method != method) continue;
        
        // 简单的路径匹配（不支持路径参数）
        if (route.path == path) {
            return route.handler;
        }
    }
    
    return nullptr;
}