// HTTP控制器基类
#ifndef HTTP_CONTROLLER_H
#define HTTP_CONTROLLER_H

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <chrono>

// HTTP请求和响应结构体
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> queryParams;
    std::unordered_map<std::string, std::string> pathParams;
};

struct HttpResponse {
    int statusCode;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    
    HttpResponse() : statusCode(200) {
        headers["Content-Type"] = "application/json";
    }
};

// 路由处理器函数类型
typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

// 路由结构体
struct Route {
    std::string method;
    std::string pathPattern;
    RouteHandler handler;
};

// HTTP控制器基类
class HttpController {
private:
    std::vector<Route> routes;
    
    // 从路径中提取参数
    std::unordered_map<std::string, std::string> extractPathParams(const std::string& path, const std::string& pattern);
    
    // 检查路径是否匹配模式
    bool matchPath(const std::string& path, const std::string& pattern);
    
    // 记录请求处理时间
    void recordRequestLatency(const std::string& endpoint, long latencyMs);
    
protected:
    // 辅助方法：创建成功响应
    HttpResponse createSuccessResponse(const std::string& body);
    
    // 辅助方法：创建错误响应
    HttpResponse createErrorResponse(int statusCode, const std::string& message);
    
    // 辅助方法：解析JSON请求体
    std::unordered_map<std::string, std::string> parseJsonBody(const std::string& jsonBody);
    
    // 辅助方法：构建JSON响应
    std::string buildJsonResponse(const std::unordered_map<std::string, std::string>& data);
    
    // 辅助方法：获取URL参数
    std::string getParam(const HttpRequest& request, const std::string& paramName, const std::string& defaultValue = "");
    
    // 辅助方法：获取路径参数中的整数
    int getIntParam(const HttpRequest& request, const std::string& paramName, int defaultValue = 0);
    
    // 辅助方法：获取路径参数中的浮点数
    float getFloatParam(const HttpRequest& request, const std::string& paramName, float defaultValue = 0.0f);
    
    // 辅助方法：验证请求方法
    bool validateMethod(const HttpRequest& request, const std::string& expectedMethod);
    
public:
    HttpController();
    virtual ~HttpController();
    
    // 注册路由
    void registerRoute(const std::string& method, const std::string& pathPattern, RouteHandler handler);
    
    // 处理请求（由HTTP服务器调用）
    HttpResponse handleRequest(const HttpRequest& request);
    
    // 获取所有路由
    const std::vector<Route>& getRoutes() const;
    
    // 初始化控制器（由子类实现）
    virtual void initialize() = 0;
};

#endif // HTTP_CONTROLLER_H
