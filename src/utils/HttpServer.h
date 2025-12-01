#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

// 简单的HTTP请求结构体
struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::string body;
};

// 简单的HTTP响应结构体
struct HttpResponse {
    int statusCode;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse(int code = 200, const std::string& contentType = "application/json")
        : statusCode(code) {
        headers["Content-Type"] = contentType;
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

// 请求处理函数类型
typedef std::function<HttpResponse(const HttpRequest&)> RequestHandler;

// 简单的HTTP服务器类
class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 注册路由
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    
    // 注册错误处理函数
    void setErrorHandler(std::function<HttpResponse(const HttpRequest&, const std::exception&)> handler);
    
private:
    int port_;
    bool running_;
    int serverSocket_;
    std::vector<int> clientSockets_;
    
    // 路由表
    struct Route {
        std::string method;
        std::string path;
        RequestHandler handler;
    };
    std::vector<Route> routes_;
    
    // 错误处理函数
    std::function<HttpResponse(const HttpRequest&, const std::exception&)> errorHandler_;
    
    // 处理客户端连接
    void handleClient(int clientSocket);
    
    // 解析HTTP请求
    HttpRequest parseRequest(const std::string& requestStr);
    
    // 格式化HTTP响应
    std::string formatResponse(const HttpResponse& response);
    
    // 匹配路由
    RequestHandler matchRoute(const std::string& method, const std::string& path, std::unordered_map<std::string, std::string>& params);
};

#endif // HTTP_SERVER_H