// 路由管理器实现
#include "router.h"
#include "user_controller.h"
#include "matching_controller.h"
#include "system_controller.h"
#include "../utils/logger.h"

Router::Router() {
}

Router::~Router() {
    // 智能指针自动管理控制器的生命周期
}

void Router::initialize() {
    // 创建并初始化用户控制器
    auto userController = std::make_unique<UserController>();
    userController->initialize();
    controllers.push_back(std::move(userController));
    
    // 创建并初始化匹配控制器
    auto matchingController = std::make_unique<MatchingController>();
    matchingController->initialize();
    controllers.push_back(std::move(matchingController));
    
    // 创建并初始化系统控制器
    auto systemController = std::make_unique<SystemController>();
    systemController->initialize();
    controllers.push_back(std::move(systemController));
    
    // 记录已注册的路由数量
    size_t totalRoutes = getAllRoutes().size();
    Logger::getInstance()->info("Router initialized with " + std::to_string(totalRoutes) + " routes");
}

HttpResponse Router::handleRequest(const HttpRequest& request) {
    // 遍历所有控制器，查找匹配的路由
    for (const auto& controller : controllers) {
        const auto& routes = controller->getRoutes();
        
        for (const auto& route : routes) {
            // 检查HTTP方法是否匹配
            if (route.method == request.method) {
                // 使用控制器的路径匹配方法检查路径是否匹配
                if (controller->matchPath(request.path, route.pathPattern)) {
                    // 记录请求信息
                    Logger::getInstance()->debug("Routing request: " + request.method + " " + request.path + " to " + route.pathPattern);
                    
                    // 调用控制器的handleRequest方法处理请求
                    // 注意：这里需要重新构造一个HttpRequest，因为原始请求会在控制器内部被修改
                    return controller->handleRequest(request);
                }
            }
        }
    }
    
    // 没有找到匹配的路由
    Logger::getInstance()->warning("No route found for: " + request.method + " " + request.path);
    
    // 返回404响应
    HttpResponse response;
    response.statusCode = 404;
    response.body = "{\"error\":\"Route not found: \" + request.method + " " + request.path + "\"}";
    response.headers["Content-Type"] = "application/json";
    return response;
}

std::vector<Route> Router::getAllRoutes() const {
    std::vector<Route> allRoutes;
    
    // 收集所有控制器的路由
    for (const auto& controller : controllers) {
        const auto& routes = controller->getRoutes();
        allRoutes.insert(allRoutes.end(), routes.begin(), routes.end());
    }
    
    return allRoutes;
}
