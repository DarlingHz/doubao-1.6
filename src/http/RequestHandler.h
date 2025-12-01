#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "http/Router.h"
#include "service/UserService.h"
#include "service/MovieService.h"
#include "service/WatchRecordService.h"
#include "service/StatisticsService.h"
#include "service/RecommendationService.h"
#include "utils/Logger.h"
#include <memory>
#include <string>

namespace http {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> pathParams;
};

struct HttpResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
    
    HttpResponse(int code = 200, const std::string& b = "")
        : status_code(code), body(b) {
        headers["Content-Type"] = "application/json";
        headers["Access-Control-Allow-Origin"] = "*";
    }
};

class RequestHandler {
public:
    RequestHandler(std::shared_ptr<service::UserService> user_service,
                  std::shared_ptr<service::MovieService> movie_service,
                  std::shared_ptr<service::WatchRecordService> watch_record_service,
                  std::shared_ptr<service::StatisticsService> statistics_service,
                  std::shared_ptr<service::RecommendationService> recommendation_service,
                  std::shared_ptr<utils::Logger> logger);
    
    // 处理HTTP请求
    std::string handleRequest(const std::string& request_str);
    
private:
    std::shared_ptr<service::UserService> user_service_;
    std::shared_ptr<service::MovieService> movie_service_;
    std::shared_ptr<service::WatchRecordService> watch_record_service_;
    std::shared_ptr<service::StatisticsService> statistics_service_;
    std::shared_ptr<service::RecommendationService> recommendation_service_;
    std::shared_ptr<utils::Logger> logger_;
    
    // 路由处理器
    Router router_;
    
    // 解析HTTP请求
    HttpRequest parseRequest(const std::string& request_str);
    
    // 构建HTTP响应
    std::string buildResponse(const HttpResponse& response);
    
    // 创建成功响应
    HttpResponse createSuccessResponse(const std::string& data);
    
    // 创建错误响应
    HttpResponse createErrorResponse(int code, const std::string& message);
    
    // 注册路由
    void registerRoutes();
    
    // 路由处理函数
    HttpResponse handleCreateUser(const HttpRequest& request);
    HttpResponse handleGetUser(const HttpRequest& request);
    HttpResponse handleCreateMovie(const HttpRequest& request);
    HttpResponse handleGetMovie(const HttpRequest& request);
    HttpResponse handleGetMovies(const HttpRequest& request);
    HttpResponse handleUpdateMovie(const HttpRequest& request);
    HttpResponse handleDeleteMovie(const HttpRequest& request);
    HttpResponse handleCreateWatchRecord(const HttpRequest& request);
    HttpResponse handleGetUserWatchRecords(const HttpRequest& request);
    HttpResponse handleGetUserStatistics(const HttpRequest& request);
    HttpResponse handleGetUserRecommendations(const HttpRequest& request);
};

} // namespace http

#endif // REQUEST_HANDLER_H
