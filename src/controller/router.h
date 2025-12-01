// 路由管理器 - 集成所有控制器的路由，统一处理HTTP请求分发
#ifndef ROUTER_H
#define ROUTER_H

#include "http_controller.h"
#include <vector>
#include <memory>

class Router {
private:
    // 所有控制器实例
    std::vector<std::unique_ptr<HttpController>> controllers;
    
public:
    Router();
    ~Router();
    
    // 初始化所有控制器和路由
    void initialize();
    
    // 处理HTTP请求，根据路径和方法分发到对应的控制器
    HttpResponse handleRequest(const HttpRequest& request);
    
    // 获取所有注册的路由信息
    std::vector<Route> getAllRoutes() const;
};

#endif // ROUTER_H
