#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <memory>
#include <optional>
#include "models.h"

// 数据库连接类
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& db_path);
    ~DatabaseConnection();
    
    // 禁止拷贝
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    
    // 允许移动
    DatabaseConnection(DatabaseConnection&& other) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&& other) noexcept;
    
    sqlite3* getConnection();
    void execute(const std::string& sql);
    
private:
    sqlite3* db_;
};

// 数据库连接池
class DatabaseConnectionPool {
public:
    DatabaseConnectionPool(const std::string& db_path, size_t pool_size = 10);
    ~DatabaseConnectionPool();
    
    std::shared_ptr<DatabaseConnection> getConnection();
    void returnConnection(std::shared_ptr<DatabaseConnection> conn);
    
private:
    std::string db_path_;
    size_t pool_size_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<DatabaseConnection>> connections_;
};

// 数据库操作接口
class Database {
public:
    Database(const std::string& db_path);
    ~Database();
    
    // 初始化数据库（执行schema）
    void initialize();
    
    // 用户相关操作
    bool createUser(const std::string& name, const std::string& email, const std::string& password_hash, int& user_id);
    std::optional<User> getUserByEmail(const std::string& email);
    std::optional<User> getUserById(int id);
    
    // 项目相关操作
    bool createProject(const Project& project);
    std::vector<Project> getUserProjects(int user_id, const PaginationParams& pagination);
    std::optional<Project> getProjectById(int project_id);
    bool isUserProjectOwner(int user_id, int project_id);
    
    // 任务相关操作
    bool createTask(const Task& task);
    std::vector<Task> getTasksByProject(int project_id, const TaskQueryParams& params, const PaginationParams& pagination);
    std::optional<Task> getTaskById(int task_id);
    bool updateTask(const Task& task);
    std::vector<Task> searchTasks(int user_id, const TaskQueryParams& params, const PaginationParams& pagination);
    
    // 标签相关操作
    int getOrCreateTag(const std::string& tag_name);
    std::vector<Tag> getAllTags();
    bool addTagToTask(int task_id, int tag_id);
    std::vector<std::string> getTaskTags(int task_id);
    
    // 审计日志
    void addAuditLog(const AuditLog& log);
    std::vector<AuditLog> getUserAuditLogs(int user_id, int limit);
    
    // 统计
    StatsOverview getUserStats(int user_id);
    
private:
    std::shared_ptr<DatabaseConnectionPool> connection_pool_;
    
    // 辅助方法
    TaskStatus stringToTaskStatus(const std::string& status);
    std::string taskStatusToString(TaskStatus status);
    TaskPriority stringToTaskPriority(const std::string& priority);
    std::string taskPriorityToString(TaskPriority priority);
    std::time_t stringToTime(const std::string& time_str);
    std::string timeToString(std::time_t time);
};

#endif // DATABASE_H