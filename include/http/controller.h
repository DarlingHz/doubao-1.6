#pragma once

#include <string>
#include <memory>
#include "external/httplib.h"
#include "external/json.hpp"
#include "service/link_service.h"
#include "utils/logger.h"

namespace http {

class Controller {
public:
    // 构造函数
    explicit Controller(std::shared_ptr<service::LinkService> linkService);
    
    // 析构函数
    ~Controller();
    
    // 注册所有路由
    void registerRoutes(httplib::Server& server);
    
private:
    // 创建短链接
    void handleCreateShortLink(const httplib::Request& req, httplib::Response& res);
    
    // 短链接重定向
    void handleRedirect(const httplib::Request& req, httplib::Response& res);
    
    // 获取短链接统计信息
    void handleGetLinkStats(const httplib::Request& req, httplib::Response& res);
    
    // 禁用短链接
    void handleDisableLink(const httplib::Request& req, httplib::Response& res);
    
    // 健康检查
    void handleHealthCheck(const httplib::Request& req, httplib::Response& res);
    
    // 404处理
    void handleNotFound(const httplib::Request& req, httplib::Response& res);
    
    // 发送JSON响应
    void sendJsonResponse(httplib::Response& res, int statusCode, const nlohmann::json& data);
    
    // 发送错误响应
    void sendErrorResponse(httplib::Response& res, int statusCode, const std::string& message);
    
    // 解析JSON请求体
    bool parseJsonRequest(const httplib::Request& req, nlohmann::json& outJson);
    
    // 从请求中获取IP地址
    std::string getClientIp(const httplib::Request& req);
    
private:
    std::shared_ptr<service::LinkService> linkService_;  // 短链接服务
};

} // namespace http
