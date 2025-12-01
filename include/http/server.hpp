#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include "http/http_router.hpp"

namespace api_quota {
namespace http {

class Server {
public:
    Server(const std::string& address,
           uint16_t port,
           std::shared_ptr<service::ClientService> client_service,
           std::shared_ptr<service::ApiKeyService> api_key_service,
           std::shared_ptr<service::QuotaService> quota_service);
    
    ~Server();
    
    // 禁止拷贝和移动
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 检查服务器是否正在运行
    bool is_running() const;
    
private:
    // 服务器主循环
    void server_loop();
    
    // 处理客户端连接
    void handle_client(int client_socket);
    
    // 解析HTTP请求
    std::unique_ptr<HttpRequest> parse_request(int client_socket);
    
    // 发送HTTP响应
    void send_response(int client_socket, const HttpResponse& response);
    
    // 关闭服务器套接字
    void close_server_socket();
    
    // 成员变量
    std::string address_;
    uint16_t port_;
    int server_socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::shared_ptr<HttpRouter> router_;
};

} // namespace http
} // namespace api_quota

#endif // SERVER_HPP