#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <optional>
#include <ctime>

// 用户模型
struct User {
    int id;
    std::string name;
    std::string email;
    std::string password_hash;
    std::time_t created_at;
};

// 项目模型
struct Project {
    int id;
    int owner_user_id;
    std::string name;
    std::string description;
    std::time_t created_at;
};

// 任务状态枚举
enum class TaskStatus {
    TODO,
    DOING,
    DONE
};

// 任务优先级枚举
enum class TaskPriority {
    LOW,
    MEDIUM,
    HIGH
};

// 任务模型
struct Task {
    int id;
    int project_id;
    std::optional<int> assignee_user_id;
    std::string title;
    std::string description;
    TaskStatus status;
    TaskPriority priority;
    std::optional<std::time_t> due_date;
    std::time_t created_at;
    std::time_t updated_at;
    std::vector<std::string> tags; // 标签名称列表
};

// 标签模型
struct Tag {
    int id;
    std::string name;
};

// 任务标签关联
struct TaskTag {
    int task_id;
    int tag_id;
};

// 审计日志模型
struct AuditLog {
    int id;
    std::optional<int> user_id;
    std::string action_type;
    std::string resource_type;
    std::optional<int> resource_id;
    std::time_t created_at;
    std::string detail;
};

// 分页参数
struct PaginationParams {
    int page;
    int page_size;
};

// 任务查询参数
struct TaskQueryParams {
    std::optional<TaskStatus> status;
    std::optional<TaskPriority> priority;
    std::optional<int> assignee_user_id;
    std::optional<std::time_t> due_before;
    std::optional<std::time_t> due_after;
    std::optional<std::string> keyword;
    std::optional<std::string> tag;
};

// 统计概览
struct StatsOverview {
    int todo_count;
    int doing_count;
    int done_count;
    int overdue_count;
    int created_last_7_days;
};

// 用户认证信息
struct UserAuth {
    std::string access_token;
    User user;
};

#endif // MODELS_H