// 匹配控制器 - 处理出行请求和匹配相关的HTTP请求
#ifndef MATCHING_CONTROLLER_H
#define MATCHING_CONTROLLER_H

#include "http_controller.h"

class MatchingController : public HttpController {
private:
    // 匹配服务依赖（前向声明）
    class MatchingService* matchingService;
    class UserService* userService;
    
    // 出行请求相关路由处理函数
    HttpResponse handleCreateTravelRequest(const HttpRequest& request);
    HttpResponse handleGetTravelRequest(const HttpRequest& request);
    HttpResponse handleUpdateTravelRequest(const HttpRequest& request);
    HttpResponse handleCancelTravelRequest(const HttpRequest& request);
    HttpResponse handleGetTravelRequestsByRider(const HttpRequest& request);
    
    // 匹配相关路由处理函数
    HttpResponse handleTriggerMatching(const HttpRequest& request);
    HttpResponse handleGetMatchingResult(const HttpRequest& request);
    HttpResponse handleAcceptMatching(const HttpRequest& request);
    HttpResponse handleRejectMatching(const HttpRequest& request);
    
    // 行程相关路由处理函数
    HttpResponse handleCreateTrip(const HttpRequest& request);
    HttpResponse handleGetTrip(const HttpRequest& request);
    HttpResponse handleUpdateTripStatus(const HttpRequest& request);
    HttpResponse handleGetTripsByRider(const HttpRequest& request);
    HttpResponse handleGetTripsByDriver(const HttpRequest& request);
    HttpResponse handleCompleteTrip(const HttpRequest& request);
    
    // 辅助方法
    bool validateTravelRequest(const std::unordered_map<std::string, std::string>& data);
    bool validateTripRequest(const std::unordered_map<std::string, std::string>& data);
    
    // 对象转JSON方法
    std::string travelRequestToJson(const class TravelRequest* request);
    std::string matchingResultToJson(const class MatchingResult* result);
    std::string tripToJson(const class Trip* trip);
    
public:
    MatchingController();
    ~MatchingController() override;
    
    // 初始化控制器，注册路由
    void initialize() override;
};

#endif // MATCHING_CONTROLLER_H
