// 系统控制器 - 处理系统统计和监控相关的HTTP请求
#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include "http_controller.h"

class SystemController : public HttpController {
private:
    // 统计监控服务依赖（前向声明）
    class StatsMonitor* statsMonitor;
    
    // 系统统计相关路由处理函数
    HttpResponse handleGetSystemStats(const HttpRequest& request);
    HttpResponse handleGetPerformanceStats(const HttpRequest& request);
    HttpResponse handleGetEndpointStats(const HttpRequest& request);
    HttpResponse handleGetActiveUsers(const HttpRequest& request);
    
    // 系统管理相关路由处理函数
    HttpResponse handleGetSystemInfo(const HttpRequest& request);
    HttpResponse handleResetStats(const HttpRequest& request);
    HttpResponse handleSetLogLevel(const HttpRequest& request);
    
public:
    SystemController();
    ~SystemController() override;
    
    // 初始化控制器，注册路由
    void initialize() override;
};

#endif // SYSTEM_CONTROLLER_H
