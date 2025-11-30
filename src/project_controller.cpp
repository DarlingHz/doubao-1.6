#include "project_controller.h"
#include <iostream>
#include <sstream>

void ProjectController::registerRoutes(HttpServer& server) {
    // 创建项目路由
    server.registerRoute("POST", "/api/v1/projects", [this](const HttpRequest& request) {
        return handleCreateProject(request);
    });
    
    // 获取项目列表路由
    server.registerRoute("GET", "/api/v1/projects", [this](const HttpRequest& request) {
        return handleGetProjects(request);
    });
    
    // 获取项目详情路由
    server.registerRoute("GET", "/api/v1/projects/([0-9]+)", [this](const HttpRequest& request) {
        return handleGetProjectDetail(request);
    });
}

HttpResponse ProjectController::handleCreateProject(const HttpRequest& request) {
    // 验证用户身份
    auto user_opt = authenticateRequest(request);
    if (!user_opt) {
        return errorResponse(401, "Unauthorized: Invalid or missing token");
    }
    
    // 解析请求体
    JsonValue body;
    if (!parseRequestBody(request, body)) {
        return errorResponse(400, "Invalid request body");
    }
    
    std::string name = body.get("name").toString();
    std::string description = body.get("description").toString();
    
    // 验证参数
    if (name.empty()) {
        return errorResponse(400, "Missing required field: name");
    }
    
    // 创建项目
    Project project;
    project.owner_user_id = user_opt->id;
    project.name = name;
    project.description = description;
    
    if (db_->createProject(project)) {
        // 获取创建的项目（假设数据库中已经设置了自增ID）
        // 在实际实现中，我们可能需要调整createProject方法来返回创建的项目
        // 这里简单起见，我们重新查询最新创建的项目
        auto projects = db_->getUserProjects(user_opt->id, {1, 1});
        if (!projects.empty()) {
            project = projects[0];
        }
        
        // 添加审计日志
        AuditLog log;
        log.user_id = user_opt->id;
        log.action_type = "create_project";
        log.resource_type = "project";
        log.resource_id = project.id;
        log.detail = "Project created: " + project.name;
        db_->addAuditLog(log);
        
        // 构造响应
        JsonValue response_data(JsonType::Object);
        response_data.add("id", JsonValue(project.id));
        response_data.add("owner_user_id", JsonValue(project.owner_user_id));
        response_data.add("name", JsonValue(project.name));
        response_data.add("description", JsonValue(project.description));
        response_data.add("created_at", JsonValue("")); // 暂时使用空字符串，后面需要实现时间格式化
        
        return successResponse(response_data);
    } else {
        return errorResponse(500, "Failed to create project");
    }
}

HttpResponse ProjectController::handleGetProjects(const HttpRequest& request) {
    // 验证用户身份
    auto user_opt = authenticateRequest(request);
    if (!user_opt) {
        return errorResponse(401, "Unauthorized: Invalid or missing token");
    }
    
    // 解析分页参数
    PaginationParams pagination = parsePaginationParams(request);
    
    // 获取用户的项目列表
    std::vector<Project> projects = db_->getUserProjects(user_opt->id, pagination);
    
    // 构造响应
    JsonValue response_data(JsonType::Array);
    for (const auto& project : projects) {
        JsonValue project_data(JsonType::Object);
        project_data.add("id", JsonValue(project.id));
        project_data.add("owner_user_id", JsonValue(project.owner_user_id));
        project_data.add("name", JsonValue(project.name));
        project_data.add("description", JsonValue(project.description));
        project_data.add("created_at", JsonValue("")); // 暂时使用空字符串
        response_data.push(project_data);
    }
    
    JsonValue final_response(JsonType::Object);
    final_response.add("projects", response_data);
    
    return successResponse(final_response);
}

HttpResponse ProjectController::handleGetProjectDetail(const HttpRequest& request) {
    // 验证用户身份
    auto user_opt = authenticateRequest(request);
    if (!user_opt) {
        return errorResponse(401, "Unauthorized: Invalid or missing token");
    }
    
    // 提取项目ID
    // 简化实现，假设URL格式为 /api/v1/projects/123
    size_t last_slash = request.path.find_last_of('/');
    std::string project_id_str = request.path.substr(last_slash + 1);
    int project_id = 0;
    try {
        project_id = std::stoi(project_id_str);
    } catch (const std::exception&) {
        return errorResponse(400, "Invalid project ID");
    }
    
    // 检查权限
    // 简化实现，使用isUserProjectOwner方法
    if (!db_->isUserProjectOwner(user_opt->id, project_id)) {
        return errorResponse(403, "Forbidden: You don't have access to this project");
    }
    
    // 获取项目详情
    auto project_opt = db_->getProjectById(project_id);
    if (!project_opt) {
        return errorResponse(404, "Project not found");
    }
    
    // 获取项目统计信息
    // 这里我们可以实现一个专门的方法来获取项目统计，现在简单起见，我们计算任务数量
    // 例如：计算todo、doing、done任务数量
    TaskQueryParams todo_params;
    
    // 构造响应
    JsonValue response_data(JsonType::Object);
    const auto& project = *project_opt;
    response_data.add("id", JsonValue(project.id));
    response_data.add("owner_user_id", JsonValue(project.owner_user_id));
    response_data.add("name", JsonValue(project.name));
    response_data.add("description", JsonValue(project.description));
    response_data.add("created_at", JsonValue("")); // 暂时使用空字符串
    
    return successResponse(response_data);
}

PaginationParams ProjectController::parsePaginationParams(const HttpRequest& request) {
    PaginationParams pagination;
    pagination.page = 1;
    pagination.page_size = 10;
    
    // 从查询参数中解析分页信息
    auto page_it = request.query_params.find("page");
    if (page_it != request.query_params.end()) {
        try {
            pagination.page = std::stoi(page_it->second);
            if (pagination.page < 1) pagination.page = 1;
        } catch (...) {
            // 忽略无效参数
        }
    }
    
    auto size_it = request.query_params.find("page_size");
    if (size_it != request.query_params.end()) {
        try {
            pagination.page_size = std::stoi(size_it->second);
            if (pagination.page_size < 1) pagination.page_size = 1;
            if (pagination.page_size > 100) pagination.page_size = 100; // 设置上限
        } catch (...) {
            // 忽略无效参数
        }
    }
    
    return pagination;
}

