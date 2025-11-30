#ifndef USER_CONTROLLER_H
#define USER_CONTROLLER_H

#include "http_server.h"

// 用户控制器类
class UserController : public Controller {
public:
    UserController(std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service)
        : Controller(db, auth_service) {}
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    // 处理用户注册
    HttpResponse handleRegister(const HttpRequest& request);
    
    // 处理用户登录
    HttpResponse handleLogin(const HttpRequest& request);
    
    // 验证注册请求参数
    bool validateRegisterRequest(const json& body, std::string& error_message);
    
    // 验证登录请求参数
    bool validateLoginRequest(const json& body, std::string& error_message);
};

#endif // USER_CONTROLLER_H