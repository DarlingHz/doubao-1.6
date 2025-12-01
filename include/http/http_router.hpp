#ifndef HTTP_ROUTER_HPP
#define HTTP_ROUTER_HPP

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "service/client_service.hpp"
#include "service/api_key_service.hpp"
#include "service/quota_service.hpp"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <regex>
#include "utils/json.hpp" // 使用项目自己的JSON实现

namespace api_quota {
namespace http {

// 路由处理函数类型
using HandlerFunction = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string method;
    std::string path;
    std::regex path_regex;
    std::vector<std::string> path_param_names;
    HandlerFunction handler;
};

class HttpRouter {
public:
    HttpRouter(std::shared_ptr<service::ClientService> client_service,
              std::shared_ptr<service::ApiKeyService> api_key_service,
              std::shared_ptr<service::QuotaService> quota_service);
    ~HttpRouter() = default;
    
    // 注册路由
    void add_route(const std::string& method, const std::string& path, HandlerFunction handler);
    
    // 处理请求
    HttpResponse handle_request(const HttpRequest& request);
    
    // 初始化所有路由
    void initialize_routes();
    
private:
    std::shared_ptr<service::ClientService> client_service_;
    std::shared_ptr<service::ApiKeyService> api_key_service_;
    std::shared_ptr<service::QuotaService> quota_service_;
    std::vector<Route> routes_;
    
    // 解析路由路径，提取路径参数名称
    std::pair<std::regex, std::vector<std::string>> parse_route_path(const std::string& path);
    
    // 尝试匹配路由
    std::optional<Route> match_route(const std::string& method, const std::string& path, HttpRequest& request);
    
    // 客户端管理路由处理函数
    HttpResponse handle_create_client(const HttpRequest& request);
    HttpResponse handle_get_clients(const HttpRequest& request);
    HttpResponse handle_get_client(const HttpRequest& request);
    HttpResponse handle_update_client(const HttpRequest& request);
    HttpResponse handle_delete_client(const HttpRequest& request);
    
    // API密钥管理路由处理函数
    HttpResponse handle_create_api_key(const HttpRequest& request);
    HttpResponse handle_get_client_api_keys(const HttpRequest& request);
    HttpResponse handle_revoke_api_key(const HttpRequest& request);
    
    // 配额校验路由处理函数
    HttpResponse handle_quota_check(const HttpRequest& request);
    
    // 统计查询路由处理函数
    HttpResponse handle_get_top_clients(const HttpRequest& request);
    HttpResponse handle_get_client_summary(const HttpRequest& request);
    HttpResponse handle_get_key_timeline(const HttpRequest& request);
    
    // 健康检查路由
    HttpResponse handle_health_check(const HttpRequest& request);
    
    // 辅助方法：解析JSON请求体
    utils::JsonValue parse_json_body(const HttpRequest& request);
    
    // 辅助方法：处理错误
    HttpResponse handle_error(HttpResponse::StatusCode status_code, const std::string& error_code, const std::string& message);
};

} // namespace http
} // namespace api_quota

#endif // HTTP_ROUTER_HPP