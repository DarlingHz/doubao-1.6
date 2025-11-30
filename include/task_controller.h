#ifndef TASK_CONTROLLER_H
#define TASK_CONTROLLER_H

#include "http_server.h"

// 任务控制器类
class TaskController : public Controller {
public:
    TaskController(std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service)
        : Controller(db, auth_service) {}
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    // 创建任务
    HttpResponse handleCreateTask(const HttpRequest& request);
    
    // 获取任务列表
    HttpResponse handleGetTasks(const HttpRequest& request);
    
    // 获取任务详情
    HttpResponse handleGetTaskDetail(const HttpRequest& request);
    
    // 更新任务
    HttpResponse handleUpdateTask(const HttpRequest& request);
    
    // 搜索任务
    HttpResponse handleSearchTasks(const HttpRequest& request);
    
    // 解析任务查询参数
    TaskQueryParams parseTaskQueryParams(const HttpRequest& request);
    
    // 验证任务状态流转
    bool validateTaskStatusTransition(TaskStatus old_status, TaskStatus new_status);
    
    // 验证任务创建请求参数
    bool validateCreateTaskRequest(const json& body, std::string& error_message);
    
    // 验证任务更新请求参数
    bool validateUpdateTaskRequest(const json& body, std::string& error_message);
};

#endif // TASK_CONTROLLER_H