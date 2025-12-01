// 用户控制器 - 处理乘客和车主相关的HTTP请求
#ifndef USER_CONTROLLER_H
#define USER_CONTROLLER_H

#include "http_controller.h"

class UserController : public HttpController {
private:
    // 用户服务依赖（前向声明）
    class UserService* userService;
    
    // 乘客相关路由处理函数
    HttpResponse handleCreateRider(const HttpRequest& request);
    HttpResponse handleGetRider(const HttpRequest& request);
    HttpResponse handleUpdateRider(const HttpRequest& request);
    HttpResponse handleDeleteRider(const HttpRequest& request);
    HttpResponse handleUpdateRiderRating(const HttpRequest& request);
    
    // 车主相关路由处理函数
    HttpResponse handleCreateDriver(const HttpRequest& request);
    HttpResponse handleGetDriver(const HttpRequest& request);
    HttpResponse handleUpdateDriver(const HttpRequest& request);
    HttpResponse handleUpdateDriverStatus(const HttpRequest& request);
    HttpResponse handleUpdateDriverLocation(const HttpRequest& request);
    HttpResponse handleUpdateDriverRating(const HttpRequest& request);
    HttpResponse handleDeleteDriver(const HttpRequest& request);
    HttpResponse handleGetAvailableDrivers(const HttpRequest& request);
    
    // 辅助方法：验证请求参数
    bool validateRiderRequest(const std::unordered_map<std::string, std::string>& data);
    bool validateDriverRequest(const std::unordered_map<std::string, std::string>& data);
    
    // 辅助方法：将用户对象转换为JSON
    std::string riderToJson(const class Rider* rider);
    std::string driverToJson(const class Driver* driver);
    
public:
    UserController();
    ~UserController() override;
    
    // 初始化控制器，注册路由
    void initialize() override;
};

#endif // USER_CONTROLLER_H
