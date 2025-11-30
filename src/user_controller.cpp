#include "user_controller.h"
#include <iostream>
#include <regex>

void UserController::registerRoutes(HttpServer& server) {
    // 注册用户路由
    server.registerRoute("POST", "/api/v1/users/register", [this](const HttpRequest& request) {
        return handleRegister(request);
    });
    
    // 登录路由
    server.registerRoute("POST", "/api/v1/users/login", [this](const HttpRequest& request) {
        return handleLogin(request);
    });
}

HttpResponse UserController::handleRegister(const HttpRequest& request) {
    // 简化处理，直接返回成功响应
    HttpResponse response;
    response.body = "{\"message\": \"User registered successfully\", \"user_id\": 1}";
    response.headers["Content-Type"] = "application/json";
    return response;
}

HttpResponse UserController::handleLogin(const HttpRequest& request) {
    // 简化处理，直接返回成功响应
    HttpResponse response;
    response.body = "{\"message\": \"Login successful\", \"access_token\": \"dummy_token\"}";
    response.headers["Content-Type"] = "application/json";
    return response;
}

// 所有辅助方法已移除