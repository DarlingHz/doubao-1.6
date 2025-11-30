#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "models.h"
#include "database.h"

// 认证服务类
class AuthService {
public:
    AuthService(std::shared_ptr<Database> db);
    
    // 用户注册
    std::optional<UserAuth> registerUser(const std::string& name, const std::string& email, const std::string& password);
    
    // 用户登录
    std::optional<UserAuth> loginUser(const std::string& email, const std::string& password);
    
    // 验证Token
    std::optional<User> verifyToken(const std::string& token);
    
    // 生成Token
    std::string generateToken(const User& user);
    
    // 检查权限：用户是否可以访问项目
    bool canAccessProject(int user_id, int project_id);
    
    // 检查权限：用户是否可以访问任务
    bool canAccessTask(int user_id, int task_id);
    
    // 检查权限：用户是否可以修改任务
    bool canModifyTask(int user_id, int task_id);
    
    // 用户登出
    bool logoutUser(const std::string& token);
    
private:
    std::shared_ptr<Database> db_;
    std::unordered_map<std::string, User> token_cache_; // 简单的内存缓存
    std::mutex cache_mutex_;
    
    // 密码哈希
    std::string hashPassword(const std::string& password);
    
    // 验证密码
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // 清理过期Token
    void cleanupExpiredTokens();
};

#endif // AUTH_H