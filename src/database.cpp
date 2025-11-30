#include "database.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

// DatabaseConnection 实现
DatabaseConnection::DatabaseConnection(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
    }
}

DatabaseConnection::~DatabaseConnection() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

sqlite3* DatabaseConnection::getConnection() {
    return db_;
}

void DatabaseConnection::execute(const std::string& sql) {
    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error("SQL error: " + error);
    }
}

// DatabaseConnectionPool 实现
DatabaseConnectionPool::DatabaseConnectionPool(const std::string& db_path, size_t pool_size) 
    : db_path_(db_path), pool_size_(pool_size) {
    // 初始化连接池
    for (size_t i = 0; i < pool_size_; ++i) {
        try {
            connections_.push(std::make_shared<DatabaseConnection>(db_path_));
        } catch (const std::exception& e) {
            std::cerr << "Failed to create database connection: " << e.what() << std::endl;
        }
    }
}

DatabaseConnectionPool::~DatabaseConnectionPool() {
    // 清理所有连接
    while (!connections_.empty()) {
        connections_.pop();
    }
}

std::shared_ptr<DatabaseConnection> DatabaseConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connections_.empty()) {
        // 如果没有可用连接，创建一个新的
        return std::make_shared<DatabaseConnection>(db_path_);
    }
    
    auto conn = connections_.front();
    connections_.pop();
    return conn;
}

void DatabaseConnectionPool::returnConnection(std::shared_ptr<DatabaseConnection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push(conn);
}

// Database 实现
Database::Database(const std::string& db_path) {
    connection_pool_ = std::make_shared<DatabaseConnectionPool>(db_path);
}

Database::~Database() {
}

void Database::initialize() {
    // 读取并执行schema.sql
    std::ifstream schema_file("db/schema.sql");
    if (!schema_file.is_open()) {
        throw std::runtime_error("Failed to open schema.sql");
    }
    
    std::stringstream buffer;
    buffer << schema_file.rdbuf();
    std::string schema = buffer.str();
    
    auto conn = connection_pool_->getConnection();
    conn->execute(schema);
    connection_pool_->returnConnection(conn);
    
    std::cout << "Database initialized successfully" << std::endl;
}

bool Database::createUser(const std::string& name, const std::string& email, const std::string& password_hash, int& user_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "INSERT INTO users (name, email, password_hash) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (success) {
        user_id = sqlite3_last_insert_rowid(db);
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return success;
}

std::optional<User> Database::getUserByEmail(const std::string& email) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, name, email, password_hash, created_at FROM users WHERE email = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    
    std::optional<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        user = u;
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return user;
}

std::optional<User> Database::getUserById(int id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, name, email, password_hash, created_at FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    std::optional<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        user = u;
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return user;
}

bool Database::createProject(const Project& project) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "INSERT INTO projects (owner_user_id, name, description) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, project.owner_user_id);
    sqlite3_bind_text(stmt, 2, project.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, project.description.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return success;
}

std::vector<Project> Database::getUserProjects(int user_id, const PaginationParams& pagination) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    std::string sql = "SELECT id, owner_user_id, name, description, created_at FROM projects WHERE owner_user_id = ? ";
    sql += "LIMIT ? OFFSET ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, pagination.page_size);
    sqlite3_bind_int(stmt, 3, (pagination.page - 1) * pagination.page_size);
    
    std::vector<Project> projects;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int(stmt, 0);
        p.owner_user_id = sqlite3_column_int(stmt, 1);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        p.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        p.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        projects.push_back(p);
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return projects;
}

std::optional<Project> Database::getProjectById(int project_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, owner_user_id, name, description, created_at FROM projects WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, project_id);
    
    std::optional<Project> project;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int(stmt, 0);
        p.owner_user_id = sqlite3_column_int(stmt, 1);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        p.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        p.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        project = p;
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return project;
}

bool Database::isUserProjectOwner(int user_id, int project_id) {
    auto project = getProjectById(project_id);
    return project && project->owner_user_id == user_id;
}

bool Database::createTask(const Task& task) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    // 开始事务
    conn->execute("BEGIN TRANSACTION");
    
    try {
        // 创建任务
        const char* task_sql = "INSERT INTO tasks (project_id, assignee_user_id, title, description, status, priority, due_date) VALUES (?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* task_stmt;
        
        if (sqlite3_prepare_v2(db, task_sql, -1, &task_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare task insert statement");
        }
        
        sqlite3_bind_int(task_stmt, 1, task.project_id);
        if (task.assignee_user_id) {
            sqlite3_bind_int(task_stmt, 2, *task.assignee_user_id);
        } else {
            sqlite3_bind_null(task_stmt, 2);
        }
        sqlite3_bind_text(task_stmt, 3, task.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 4, task.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 5, taskStatusToString(task.status).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 6, taskPriorityToString(task.priority).c_str(), -1, SQLITE_TRANSIENT);
        if (task.due_date) {
            sqlite3_bind_text(task_stmt, 7, timeToString(*task.due_date).c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(task_stmt, 7);
        }
        
        if (sqlite3_step(task_stmt) != SQLITE_DONE) {
            sqlite3_finalize(task_stmt);
            throw std::runtime_error("Failed to insert task");
        }
        
        int task_id = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(task_stmt);
        
        // 添加标签关联
        for (const auto& tag_name : task.tags) {
            int tag_id = getOrCreateTag(tag_name);
            addTagToTask(task_id, tag_id);
        }
        
        // 提交事务
        conn->execute("COMMIT");
        connection_pool_->returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        // 回滚事务
        conn->execute("ROLLBACK");
        connection_pool_->returnConnection(conn);
        std::cerr << "Error creating task: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Task> Database::getTasksByProject(int project_id, const TaskQueryParams& params, const PaginationParams& pagination) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    std::string sql = "SELECT t.id, t.project_id, t.assignee_user_id, t.title, t.description, t.status, t.priority, t.due_date, t.created_at, t.updated_at " 
                     "FROM tasks t WHERE t.project_id = ?";
    
    // 添加过滤条件
    std::vector<std::string> filters;
    if (params.status) {
        filters.push_back("t.status = '" + taskStatusToString(*params.status) + "'");
    }
    if (params.priority) {
        filters.push_back("t.priority = '" + taskPriorityToString(*params.priority) + "'");
    }
    if (params.assignee_user_id) {
        filters.push_back("t.assignee_user_id = " + std::to_string(*params.assignee_user_id));
    }
    if (params.due_before) {
        filters.push_back("t.due_date <= '" + timeToString(*params.due_before) + "'");
    }
    if (params.due_after) {
        filters.push_back("t.due_date >= '" + timeToString(*params.due_after) + "'");
    }
    
    for (const auto& filter : filters) {
        sql += " AND " + filter;
    }
    
    sql += " ORDER BY t.created_at DESC LIMIT ? OFFSET ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, pagination.page_size);
    sqlite3_bind_int(stmt, 3, (pagination.page - 1) * pagination.page_size);
    
    std::vector<Task> tasks;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Task task;
        task.id = sqlite3_column_int(stmt, 0);
        task.project_id = sqlite3_column_int(stmt, 1);
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            task.assignee_user_id = sqlite3_column_int(stmt, 2);
        }
        task.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        task.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        task.status = stringToTaskStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        task.priority = stringToTaskPriority(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            task.due_date = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        }
        task.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        task.updated_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        
        // 获取任务标签
        task.tags = getTaskTags(task.id);
        
        tasks.push_back(task);
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return tasks;
}

std::optional<Task> Database::getTaskById(int task_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, project_id, assignee_user_id, title, description, status, priority, due_date, created_at, updated_at FROM tasks WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, task_id);
    
    std::optional<Task> task;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Task t;
        t.id = sqlite3_column_int(stmt, 0);
        t.project_id = sqlite3_column_int(stmt, 1);
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            t.assignee_user_id = sqlite3_column_int(stmt, 2);
        }
        t.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        t.status = stringToTaskStatus(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        t.priority = stringToTaskPriority(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            t.due_date = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        }
        t.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        t.updated_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        
        // 获取任务标签
        t.tags = getTaskTags(t.id);
        
        task = t;
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return task;
}

bool Database::updateTask(const Task& task) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    // 开始事务
    conn->execute("BEGIN TRANSACTION");
    
    try {
        // 更新任务基本信息
        const char* task_sql = "UPDATE tasks SET title = ?, description = ?, status = ?, priority = ?, assignee_user_id = ?, due_date = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
        sqlite3_stmt* task_stmt;
        
        if (sqlite3_prepare_v2(db, task_sql, -1, &task_stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare task update statement");
        }
        
        sqlite3_bind_text(task_stmt, 1, task.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 2, task.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 3, taskStatusToString(task.status).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(task_stmt, 4, taskPriorityToString(task.priority).c_str(), -1, SQLITE_TRANSIENT);
        if (task.assignee_user_id) {
            sqlite3_bind_int(task_stmt, 5, *task.assignee_user_id);
        } else {
            sqlite3_bind_null(task_stmt, 5);
        }
        if (task.due_date) {
            sqlite3_bind_text(task_stmt, 6, timeToString(*task.due_date).c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(task_stmt, 6);
        }
        sqlite3_bind_int(task_stmt, 7, task.id);
        
        if (sqlite3_step(task_stmt) != SQLITE_DONE) {
            sqlite3_finalize(task_stmt);
            throw std::runtime_error("Failed to update task");
        }
        
        sqlite3_finalize(task_stmt);
        
        // 删除旧的标签关联
        conn->execute("DELETE FROM task_tags WHERE task_id = " + std::to_string(task.id));
        
        // 添加新的标签关联
        for (const auto& tag_name : task.tags) {
            int tag_id = getOrCreateTag(tag_name);
            addTagToTask(task.id, tag_id);
        }
        
        // 提交事务
        conn->execute("COMMIT");
        connection_pool_->returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        // 回滚事务
        conn->execute("ROLLBACK");
        connection_pool_->returnConnection(conn);
        std::cerr << "Error updating task: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Task> Database::searchTasks(int user_id, const TaskQueryParams& params, const PaginationParams& pagination) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    // 先获取用户有权限的项目ID列表
    std::string project_ids_sql = "SELECT id FROM projects WHERE owner_user_id = " + std::to_string(user_id);
    sqlite3_stmt* project_stmt;
    
    if (sqlite3_prepare_v2(db, project_ids_sql.c_str(), -1, &project_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(project_stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    std::vector<int> project_ids;
    while (sqlite3_step(project_stmt) == SQLITE_ROW) {
        project_ids.push_back(sqlite3_column_int(project_stmt, 0));
    }
    sqlite3_finalize(project_stmt);
    
    if (project_ids.empty()) {
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    // 构建任务查询SQL
    std::string sql = "SELECT t.id, t.project_id, t.assignee_user_id, t.title, t.description, t.status, t.priority, t.due_date, t.created_at, t.updated_at "
                     "FROM tasks t WHERE t.project_id IN (";
    
    for (size_t i = 0; i < project_ids.size(); ++i) {
        sql += std::to_string(project_ids[i]);
        if (i < project_ids.size() - 1) {
            sql += ", ";
        }
    }
    sql += ")";
    
    // 添加过滤条件
    if (params.status) {
        sql += " AND t.status = '" + taskStatusToString(*params.status) + "'";
    }
    if (params.priority) {
        sql += " AND t.priority = '" + taskPriorityToString(*params.priority) + "'";
    }
    if (params.assignee_user_id) {
        sql += " AND t.assignee_user_id = " + std::to_string(*params.assignee_user_id);
    }
    if (params.due_before) {
        sql += " AND t.due_date <= '" + timeToString(*params.due_before) + "'";
    }
    if (params.due_after) {
        sql += " AND t.due_date >= '" + timeToString(*params.due_after) + "'";
    }
    if (params.keyword) {
        sql += " AND (t.title LIKE '%" + *params.keyword + "%' OR t.description LIKE '%" + *params.keyword + "%')";
    }
    
    // 如果有标签过滤，需要额外处理
    if (params.tag) {
        sql = "SELECT t.id, t.project_id, t.assignee_user_id, t.title, t.description, t.status, t.priority, t.due_date, t.created_at, t.updated_at "
             "FROM tasks t "
             "JOIN task_tags tt ON t.id = tt.task_id "
             "JOIN tags tag ON tt.tag_id = tag.id "
             "WHERE t.project_id IN (";
        
        for (size_t i = 0; i < project_ids.size(); ++i) {
            sql += std::to_string(project_ids[i]);
            if (i < project_ids.size() - 1) {
                sql += ", ";
            }
        }
        sql += ") AND tag.name = '" + *params.tag + "'";
        
        // 添加其他过滤条件
        if (params.status) {
            sql += " AND t.status = '" + taskStatusToString(*params.status) + "'";
        }
        if (params.priority) {
            sql += " AND t.priority = '" + taskPriorityToString(*params.priority) + "'";
        }
        if (params.assignee_user_id) {
            sql += " AND t.assignee_user_id = " + std::to_string(*params.assignee_user_id);
        }
        if (params.due_before) {
            sql += " AND t.due_date <= '" + timeToString(*params.due_before) + "'";
        }
        if (params.due_after) {
            sql += " AND t.due_date >= '" + timeToString(*params.due_after) + "'";
        }
        if (params.keyword) {
            sql += " AND (t.title LIKE '%" + *params.keyword + "%' OR t.description LIKE '%" + *params.keyword + "%')";
        }
    }
    
    sql += " GROUP BY t.id ORDER BY t.created_at DESC LIMIT ? OFFSET ?";
    
    sqlite3_stmt* task_stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &task_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(task_stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    sqlite3_bind_int(task_stmt, 1, pagination.page_size);
    sqlite3_bind_int(task_stmt, 2, (pagination.page - 1) * pagination.page_size);
    
    std::vector<Task> tasks;
    while (sqlite3_step(task_stmt) == SQLITE_ROW) {
        Task task;
        task.id = sqlite3_column_int(task_stmt, 0);
        task.project_id = sqlite3_column_int(task_stmt, 1);
        if (sqlite3_column_type(task_stmt, 2) != SQLITE_NULL) {
            task.assignee_user_id = sqlite3_column_int(task_stmt, 2);
        }
        task.title = reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 3));
        task.description = reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 4));
        task.status = stringToTaskStatus(reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 5)));
        task.priority = stringToTaskPriority(reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 6)));
        if (sqlite3_column_type(task_stmt, 7) != SQLITE_NULL) {
            task.due_date = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 7)));
        }
        task.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 8)));
        task.updated_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(task_stmt, 9)));
        
        // 获取任务标签
        task.tags = getTaskTags(task.id);
        
        tasks.push_back(task);
    }
    
    sqlite3_finalize(task_stmt);
    connection_pool_->returnConnection(conn);
    return tasks;
}

int Database::getOrCreateTag(const std::string& tag_name) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    // 先尝试查找标签
    const char* find_sql = "SELECT id FROM tags WHERE name = ?";
    sqlite3_stmt* find_stmt;
    
    if (sqlite3_prepare_v2(db, find_sql, -1, &find_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(find_stmt);
        connection_pool_->returnConnection(conn);
        return -1;
    }
    
    sqlite3_bind_text(find_stmt, 1, tag_name.c_str(), -1, SQLITE_TRANSIENT);
    
    int tag_id = -1;
    if (sqlite3_step(find_stmt) == SQLITE_ROW) {
        tag_id = sqlite3_column_int(find_stmt, 0);
    }
    
    sqlite3_finalize(find_stmt);
    
    // 如果标签不存在，创建它
    if (tag_id == -1) {
        const char* create_sql = "INSERT INTO tags (name) VALUES (?)";
        sqlite3_stmt* create_stmt;
        
        if (sqlite3_prepare_v2(db, create_sql, -1, &create_stmt, nullptr) != SQLITE_OK) {
            sqlite3_finalize(create_stmt);
            connection_pool_->returnConnection(conn);
            return -1;
        }
        
        sqlite3_bind_text(create_stmt, 1, tag_name.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(create_stmt) == SQLITE_DONE) {
            tag_id = sqlite3_last_insert_rowid(db);
        }
        
        sqlite3_finalize(create_stmt);
    }
    
    connection_pool_->returnConnection(conn);
    return tag_id;
}

std::vector<Tag> Database::getAllTags() {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, name FROM tags";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    std::vector<Tag> tags;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Tag tag;
        tag.id = sqlite3_column_int(stmt, 0);
        tag.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tags.push_back(tag);
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return tags;
}

bool Database::addTagToTask(int task_id, int tag_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "INSERT INTO task_tags (task_id, tag_id) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, task_id);
    sqlite3_bind_int(stmt, 2, tag_id);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return success;
}

std::vector<std::string> Database::getTaskTags(int task_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT t.name FROM tags t JOIN task_tags tt ON t.id = tt.tag_id WHERE tt.task_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, task_id);
    
    std::vector<std::string> tags;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tags.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return tags;
}

void Database::addAuditLog(const AuditLog& log) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "INSERT INTO audit_logs (user_id, action_type, resource_type, resource_id, detail) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return;
    }
    
    if (log.user_id) {
        sqlite3_bind_int(stmt, 1, *log.user_id);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_text(stmt, 2, log.action_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, log.resource_type.c_str(), -1, SQLITE_TRANSIENT);
    if (log.resource_id) {
        sqlite3_bind_int(stmt, 4, *log.resource_id);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    sqlite3_bind_text(stmt, 5, log.detail.c_str(), -1, SQLITE_TRANSIENT);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
}

std::vector<AuditLog> Database::getUserAuditLogs(int user_id, int limit) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    const char* sql = "SELECT id, user_id, action_type, resource_type, resource_id, created_at, detail FROM audit_logs WHERE user_id = ? ORDER BY created_at DESC LIMIT ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        connection_pool_->returnConnection(conn);
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, limit);
    
    std::vector<AuditLog> logs;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AuditLog log;
        log.id = sqlite3_column_int(stmt, 0);
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            log.user_id = sqlite3_column_int(stmt, 1);
        }
        log.action_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        log.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
            log.resource_id = sqlite3_column_int(stmt, 4);
        }
        log.created_at = stringToTime(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        log.detail = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        logs.push_back(log);
    }
    
    sqlite3_finalize(stmt);
    connection_pool_->returnConnection(conn);
    return logs;
}

StatsOverview Database::getUserStats(int user_id) {
    auto conn = connection_pool_->getConnection();
    sqlite3* db = conn->getConnection();
    
    StatsOverview stats = {0, 0, 0, 0, 0};
    
    // 先获取用户有权限的项目ID列表
    std::string project_ids_sql = "SELECT id FROM projects WHERE owner_user_id = " + std::to_string(user_id);
    sqlite3_stmt* project_stmt;
    
    if (sqlite3_prepare_v2(db, project_ids_sql.c_str(), -1, &project_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(project_stmt);
        connection_pool_->returnConnection(conn);
        return stats;
    }
    
    std::vector<int> project_ids;
    while (sqlite3_step(project_stmt) == SQLITE_ROW) {
        project_ids.push_back(sqlite3_column_int(project_stmt, 0));
    }
    sqlite3_finalize(project_stmt);
    
    if (project_ids.empty()) {
        connection_pool_->returnConnection(conn);
        return stats;
    }
    
    // 构建项目ID列表字符串
    std::string project_ids_str;
    for (size_t i = 0; i < project_ids.size(); ++i) {
        project_ids_str += std::to_string(project_ids[i]);
        if (i < project_ids.size() - 1) {
            project_ids_str += ", ";
        }
    }
    
    // 获取各状态任务数量
    std::string status_sql = "SELECT status, COUNT(*) FROM tasks WHERE project_id IN (" + project_ids_str + ") GROUP BY status";
    sqlite3_stmt* status_stmt;
    
    if (sqlite3_prepare_v2(db, status_sql.c_str(), -1, &status_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(status_stmt);
        connection_pool_->returnConnection(conn);
        return stats;
    }
    
    while (sqlite3_step(status_stmt) == SQLITE_ROW) {
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(status_stmt, 0));
        int count = sqlite3_column_int(status_stmt, 1);
        
        if (status == "todo") stats.todo_count = count;
        else if (status == "doing") stats.doing_count = count;
        else if (status == "done") stats.done_count = count;
    }
    
    sqlite3_finalize(status_stmt);
    
    // 获取逾期任务数量
    std::time_t now = std::time(nullptr);
    std::string overdue_sql = "SELECT COUNT(*) FROM tasks WHERE project_id IN (" + project_ids_str + ") AND due_date < '" + timeToString(now) + "' AND status != 'done'";
    sqlite3_stmt* overdue_stmt;
    
    if (sqlite3_prepare_v2(db, overdue_sql.c_str(), -1, &overdue_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(overdue_stmt);
        connection_pool_->returnConnection(conn);
        return stats;
    }
    
    if (sqlite3_step(overdue_stmt) == SQLITE_ROW) {
        stats.overdue_count = sqlite3_column_int(overdue_stmt, 0);
    }
    
    sqlite3_finalize(overdue_stmt);
    
    // 获取近7天内新建任务数量
    std::time_t seven_days_ago = now - 7 * 24 * 60 * 60; // 7天前
    std::string recent_sql = "SELECT COUNT(*) FROM tasks WHERE project_id IN (" + project_ids_str + ") AND created_at >= '" + timeToString(seven_days_ago) + "'";
    sqlite3_stmt* recent_stmt;
    
    if (sqlite3_prepare_v2(db, recent_sql.c_str(), -1, &recent_stmt, nullptr) != SQLITE_OK) {
        sqlite3_finalize(recent_stmt);
        connection_pool_->returnConnection(conn);
        return stats;
    }
    
    if (sqlite3_step(recent_stmt) == SQLITE_ROW) {
        stats.created_last_7_days = sqlite3_column_int(recent_stmt, 0);
    }
    
    sqlite3_finalize(recent_stmt);
    connection_pool_->returnConnection(conn);
    return stats;
}

TaskStatus Database::stringToTaskStatus(const std::string& status) {
    if (status == "todo") return TaskStatus::TODO;
    if (status == "doing") return TaskStatus::DOING;
    if (status == "done") return TaskStatus::DONE;
    throw std::runtime_error("Invalid task status: " + status);
}

std::string Database::taskStatusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::TODO: return "todo";
        case TaskStatus::DOING: return "doing";
        case TaskStatus::DONE: return "done";
        default: return "unknown";
    }
}

TaskPriority Database::stringToTaskPriority(const std::string& priority) {
    if (priority == "low") return TaskPriority::LOW;
    if (priority == "medium") return TaskPriority::MEDIUM;
    if (priority == "high") return TaskPriority::HIGH;
    throw std::runtime_error("Invalid task priority: " + priority);
}

std::string Database::taskPriorityToString(TaskPriority priority) {
    switch (priority) {
        case TaskPriority::LOW: return "low";
        case TaskPriority::MEDIUM: return "medium";
        case TaskPriority::HIGH: return "high";
        default: return "unknown";
    }
}

std::time_t Database::stringToTime(const std::string& time_str) {
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::mktime(&tm);
}

std::string Database::timeToString(std::time_t time) {
    std::tm* tm = std::localtime(&time);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
    return buffer;
}