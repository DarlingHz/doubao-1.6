#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <memory>
#include <string>
#include "model/User.h"
#include "model/RequestResponse.h"

namespace service {

class UserService {
public:
    virtual ~UserService() = default;
    
    // 用户注册
    virtual model::ApiResponse<model::LoginResponse> registerUser(const model::RegisterRequest& request) = 0;
    
    // 用户登录
    virtual model::ApiResponse<model::LoginResponse> login(const model::LoginRequest& request) = 0;
    
    // 根据ID获取用户信息
    virtual model::ApiResponse<model::User> getUserById(int id) = 0;
    
    // 验证Token并获取用户ID
    virtual model::ApiResponse<int> validateToken(const std::string& token) = 0;
    
    // 生成JWT Token
    virtual std::string generateToken(int userId, const std::string& email) = 0;
    
    // 加密密码
    virtual std::pair<std::string, std::string> hashPassword(const std::string& password) = 0;
    
    // 验证密码
    virtual bool verifyPassword(const std::string& password, const std::string& passwordHash, const std::string& salt) = 0;
};

} // namespace service

#endif // USER_SERVICE_H