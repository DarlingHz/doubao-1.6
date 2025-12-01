#include "http/server.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_router.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <csignal>
#include <vector>

namespace api_quota {
namespace http {

// 全局变量用于存储服务器实例指针，以便在信号处理中使用
static Server* g_server_instance = nullptr;

// 信号处理函数
static void signal_handler(int signum) {
    if (g_server_instance && g_server_instance->is_running()) {
        std::cout << "Received signal " << signum << ", shutting down server..." << std::endl;
        g_server_instance->stop();
    }
}

Server::Server(const std::string& address,
               uint16_t port,
               std::shared_ptr<service::ClientService> client_service,
               std::shared_ptr<service::ApiKeyService> api_key_service,
               std::shared_ptr<service::QuotaService> quota_service)
    : address_(address),
      port_(port),
      server_socket_(-1),
      running_(false) {
    
    // 初始化路由器
    router_ = std::make_shared<HttpRouter>(client_service, api_key_service, quota_service);
    
    // 存储服务器实例指针
    g_server_instance = this;
}

Server::~Server() {
    stop();
    
    // 清除全局实例指针
    if (g_server_instance == this) {
        g_server_instance = nullptr;
    }
}

void Server::start() {
    // 设置信号处理
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    
    // 创建套接字
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    
    // 设置套接字选项
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address_.c_str());
    server_addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
    
    // 开始监听
    if (listen(server_socket_, 100) < 0) {
        throw std::runtime_error("Failed to listen: " + std::string(strerror(errno)));
    }
    
    // 设置为非阻塞模式
    int flags = fcntl(server_socket_, F_GETFL, 0);
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
    
    // 启动服务器线程
    running_ = true;
    server_thread_ = std::thread(&Server::server_loop, this);
}

void Server::stop() {
    if (running_) {
        running_ = false;
        
        // 关闭服务器套接字，唤醒阻塞的poll调用
        close_server_socket();
        
        // 等待服务器线程结束
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        std::cout << "Server stopped" << std::endl;
    }
}

bool Server::is_running() const {
    return running_;
}

void Server::server_loop() {
    std::vector<pollfd> poll_fds;
    poll_fds.push_back({server_socket_, POLLIN, 0});
    
    while (running_) {
        int poll_result = poll(poll_fds.data(), poll_fds.size(), 1000); // 1秒超时
        
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续循环
            }
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            break;
        } else if (poll_result == 0) {
            continue; // 超时，继续循环
        }
        
        // 检查是否有新的连接请求
        if (poll_fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Accept error: " << strerror(errno) << std::endl;
                }
                continue;
            }
            
            // 创建新线程处理客户端连接
            std::thread client_thread(&Server::handle_client, this, client_socket);
            client_thread.detach();
        }
    }
}

void Server::handle_client(int client_socket) {
    // 设置客户端套接字为非阻塞
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
    
    try {
        // 解析HTTP请求
        auto request = parse_request(client_socket);
        if (!request) {
            HttpResponse error_response(HttpResponse::BAD_REQUEST);
            error_response.set_json_body("{\"error\":\"invalid_request\",\"message\":\"Invalid HTTP request\"}");
            send_response(client_socket, error_response);
            close(client_socket);
            return;
        }
        
        // 处理请求
        HttpResponse response = router_->handle_request(*request);
        
        // 发送响应
        send_response(client_socket, response);
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        
        try {
            HttpResponse error_response(HttpResponse::INTERNAL_SERVER_ERROR);
            error_response.set_json_body("{\"error\":\"internal_error\",\"message\":\"Internal server error\"}");
            send_response(client_socket, error_response);
        } catch (...) {
            // 忽略响应发送错误
        }
    }
    
    // 关闭客户端套接字
    close(client_socket);
}

std::unique_ptr<HttpRequest> Server::parse_request(int client_socket) {
    const size_t BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];
    std::string request_data;
    
    // 设置poll等待接收数据
    struct pollfd poll_fd = {client_socket, POLLIN, 0};
    int poll_result = poll(&poll_fd, 1, 5000); // 5秒超时
    
    if (poll_result <= 0) {
        return nullptr; // 超时或错误
    }
    
    // 接收请求数据
    ssize_t bytes_read;
    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        request_data.append(buffer, bytes_read);
        
        // 检查是否收到完整的请求
        if (request_data.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    
    if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return nullptr;
    }
    
    if (request_data.empty()) {
        return nullptr;
    }
    
    // 解析请求行
    size_t method_end = request_data.find(' ');
    if (method_end == std::string::npos) {
        return nullptr;
    }
    
    std::string method = request_data.substr(0, method_end);
    
    size_t path_end = request_data.find(' ', method_end + 1);
    if (path_end == std::string::npos) {
        return nullptr;
    }
    
    std::string path = request_data.substr(method_end + 1, path_end - method_end - 1);
    
    // 解析请求体（如果有）
    std::string body;
    size_t body_start = request_data.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        body_start += 4; // 跳过\r\n\r\n
        // 解析Content-Length头
        size_t content_length_pos = request_data.find("Content-Length:", 0);
        if (content_length_pos != std::string::npos) {
            content_length_pos += 15; // 跳过"Content-Length: "
            size_t content_length_end = request_data.find("\r\n", content_length_pos);
            if (content_length_end != std::string::npos) {
                try {
                    size_t content_length = std::stoull(request_data.substr(content_length_pos, content_length_end - content_length_pos));
                    size_t available_body = request_data.size() - body_start;
                    
                    // 如果数据不足，尝试继续接收
                    if (available_body < content_length) {
                        // 这里简化处理，实际应该继续接收数据直到达到Content-Length
                        // 为了简化，我们只使用已有的数据
                        body = request_data.substr(body_start);
                    } else {
                        body = request_data.substr(body_start, content_length);
                    }
                } catch (...) {
                    // 解析Content-Length失败
                    body = request_data.substr(body_start);
                }
            } else {
                body = request_data.substr(body_start);
            }
        } else {
            body = request_data.substr(body_start);
        }
    }
    
    // 创建SimpleHttpRequest对象
    auto request = std::make_unique<SimpleHttpRequest>(method, path, body);
    
    // 解析请求头
    size_t headers_start = path_end + 9; // 跳过HTTP/1.x和\r\n
    if (headers_start < request_data.size()) {
        size_t headers_end = request_data.find("\r\n\r\n");
        if (headers_end != std::string::npos) {
            std::string headers_part = request_data.substr(headers_start, headers_end - headers_start);
            
            size_t pos = 0;
            while (pos < headers_part.size()) {
                size_t line_end = headers_part.find("\r\n", pos);
                if (line_end == std::string::npos) break;
                
                std::string header_line = headers_part.substr(pos, line_end - pos);
                size_t colon_pos = header_line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string header_name = header_line.substr(0, colon_pos);
                    std::string header_value = header_line.substr(colon_pos + 1);
                    
                    // 去除值的前后空白
                    size_t value_start = header_value.find_first_not_of(" ");
                    size_t value_end = header_value.find_last_not_of(" ");
                    if (value_start != std::string::npos && value_end != std::string::npos) {
                        header_value = header_value.substr(value_start, value_end - value_start + 1);
                    }
                    
                    request->set_header(header_name, header_value);
                }
                
                pos = line_end + 2; // 跳过\r\n
            }
        }
    }
    
    // 解析查询参数（如果有）
    size_t query_start = path.find('?');
    if (query_start != std::string::npos) {
        std::string query_string = path.substr(query_start + 1);
        
        size_t pos = 0;
        while (pos < query_string.size()) {
            size_t param_end = query_string.find('&', pos);
            if (param_end == std::string::npos) {
                param_end = query_string.size();
            }
            
            std::string param = query_string.substr(pos, param_end - pos);
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                std::string param_name = param.substr(0, eq_pos);
                std::string param_value = param.substr(eq_pos + 1);
                request->set_query_param(param_name, param_value);
            }
            
            pos = param_end + 1; // 跳过&
        }
    }
    
    return request;
}

void Server::send_response(int client_socket, const HttpResponse& response) {
    // 构建响应字符串
    std::string response_str = response.to_string();
    
    // 发送响应
    const char* data = response_str.c_str();
    size_t total_bytes = response_str.size();
    size_t sent_bytes = 0;
    
    while (sent_bytes < total_bytes) {
        ssize_t bytes_written = send(client_socket, data + sent_bytes, total_bytes - sent_bytes, 0);
        if (bytes_written < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to send response: " + std::string(strerror(errno)));
            }
            // 设置非阻塞发送，使用poll等待可写
            struct pollfd poll_fd = {client_socket, POLLOUT, 0};
            int poll_result = poll(&poll_fd, 1, 5000); // 5秒超时
            if (poll_result <= 0) {
                throw std::runtime_error("Failed to send response: timeout");
            }
        } else {
            sent_bytes += bytes_written;
        }
    }
}

void Server::close_server_socket() {
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

} // namespace http
} // namespace api_quota