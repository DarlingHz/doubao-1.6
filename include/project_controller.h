#ifndef PROJECT_CONTROLLER_H
#define PROJECT_CONTROLLER_H

#include "http_server.h"

// 项目控制器类
class ProjectController : public Controller {
public:
    ProjectController(std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service)
        : Controller(db, auth_service) {}
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    // 创建项目
    HttpResponse handleCreateProject(const HttpRequest& request);
    
    // 获取项目列表
    HttpResponse handleGetProjects(const HttpRequest& request);
    
    // 获取项目详情
    HttpResponse handleGetProjectDetail(const HttpRequest& request);
    
    // 解析分页参数
    PaginationParams parsePaginationParams(const HttpRequest& request);
    
    // 验证项目创建请求参数
    bool validateCreateProjectRequest(const json& body, std::string& error_message);
};

#endif // PROJECT_CONTROLLER_H