#include "auth.h"
#include <iostream>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

// 简单的密码哈希函数（实际生产环境应使用bcrypt、Argon2等）
std::string AuthService::hashPassword(const std::string& password) {
    // 这里使用简单的哈希实现，仅用于演示
    // 实际应用中应使用更强的哈希算法
    size_t hash = 0;
    for (char c : password) {
        hash = (hash * 31 + c) % 1000000007;
    }
    
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << hash;
    return ss.str();
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hashed_password) {
    return hashPassword(password) == hashed_password;
}

std::string AuthService::generateToken(const User& user) {
    // 生成基于时间戳和用户ID的随机token
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1000, 9999);
    
    std::stringstream ss;
    ss << "token_" << user.id << "_" << millis << "_" << distrib(gen);
    return ss.str();
}

AuthService::AuthService(std::shared_ptr<Database> db) : db_(db) {
}

std::optional<UserAuth> AuthService::registerUser(const std::string& name, const std::string& email, const std::string& password) {
    // 检查邮箱是否已存在
    if (db_->getUserByEmail(email)) {
        return std::nullopt; // 邮箱已注册
    }
    
    // 对密码进行哈希
    std::string hashed_password = hashPassword(password);
    
    // 创建用户
    int user_id = 0;
    if (db_->createUser(name, email, hashed_password, user_id)) {
        // 获取创建的用户
        auto user_opt = db_->getUserById(user_id);
        if (user_opt) {
            // 生成token
            std::string token = generateToken(*user_opt);
            
            // 存储token到内存缓存
            std::lock_guard<std::mutex> lock(cache_mutex_);
            token_cache_[token] = *user_opt;
            
            // 添加审计日志
            AuditLog log;
            log.user_id = user_id;
            log.action_type = "register";
            log.resource_type = "user";
            log.resource_id = user_id;
            log.detail = "User registered successfully";
            db_->addAuditLog(log);
            
            UserAuth auth;
            auth.user = *user_opt;
            auth.access_token = token;
            return auth;
        }
    }
    
    return std::nullopt;
}

std::optional<UserAuth> AuthService::loginUser(const std::string& email, const std::string& password) {
    // 查找用户
    auto user_opt = db_->getUserByEmail(email);
    if (!user_opt) {
        return std::nullopt; // 用户不存在
    }
    
    // 验证密码
    if (!verifyPassword(password, user_opt->password_hash)) {
        return std::nullopt; // 密码错误
    }
    
    // 生成新token
    std::string token = generateToken(*user_opt);
    
    // 更新token缓存
    std::lock_guard<std::mutex> lock(cache_mutex_);
    token_cache_[token] = *user_opt;
    
    // 添加审计日志
    AuditLog log;
    log.user_id = user_opt->id;
    log.action_type = "login";
    log.resource_type = "user";
    log.resource_id = user_opt->id;
    log.detail = "User logged in successfully";
    db_->addAuditLog(log);
    
    UserAuth auth;
    auth.user = *user_opt;
    auth.access_token = token;
    return auth;
}

std::optional<User> AuthService::verifyToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = token_cache_.find(token);
    if (it != token_cache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool AuthService::canAccessProject(int user_id, int project_id) {
    // 检查用户是否是项目所有者
    auto project_opt = db_->getProjectById(project_id);
    if (project_opt && project_opt->owner_user_id == user_id) {
        return true;
    }
    
    // 可以扩展检查项目成员表
    return false;
}

bool AuthService::canAccessTask(int user_id, int task_id) {
    // 获取任务信息
    auto task_opt = db_->getTaskById(task_id);
    if (!task_opt) {
        return false;
    }
    
    // 检查用户是否有权限访问任务所属的项目
    return canAccessProject(user_id, task_opt->project_id);
}

bool AuthService::canModifyTask(int user_id, int task_id) {
    // 获取任务信息
    auto task_opt = db_->getTaskById(task_id);
    if (!task_opt) {
        return false;
    }
    
    // 检查用户是否是任务的执行者
    if (task_opt->assignee_user_id && task_opt->assignee_user_id == user_id) {
        return true;
    }
    
    // 检查用户是否是项目所有者
    return canAccessProject(user_id, task_opt->project_id);
}

void AuthService::cleanupExpiredTokens() {
    // 简单的清理实现
    // 实际应用中应根据Token的过期时间进行清理
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 这里可以添加过期时间检查逻辑
    // 目前仅保留有限数量的Token
    if (token_cache_.size() > 10000) {
        // 简单地删除一半的Token
        size_t to_remove = token_cache_.size() / 2;
        auto it = token_cache_.begin();
        while (to_remove > 0 && it != token_cache_.end()) {
            it = token_cache_.erase(it);
            to_remove--;
        }
    }
}

bool AuthService::logoutUser(const std::string& token) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = token_cache_.find(token);
    if (it != token_cache_.end()) {
        int user_id = it->second.id;
        token_cache_.erase(it);
        
        // 添加审计日志
        AuditLog log;
        log.user_id = user_id;
        log.action_type = "logout";
        log.resource_type = "user";
        log.resource_id = user_id;
        log.detail = "User logged out successfully";
        db_->addAuditLog(log);
        
        return true;
    }
    
    return false;
}