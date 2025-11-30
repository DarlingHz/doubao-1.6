#ifndef STATS_CONTROLLER_H
#define STATS_CONTROLLER_H

#include "http_server.h"

// 统计和审计日志控制器类
class StatsController : public Controller {
public:
    StatsController(std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service)
        : Controller(db, auth_service) {}
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    // 获取用户统计概览
    HttpResponse handleGetStatsOverview(const HttpRequest& request);
    
    // 获取审计日志
    HttpResponse handleGetAuditLogs(const HttpRequest& request);
};

#endif // STATS_CONTROLLER_H