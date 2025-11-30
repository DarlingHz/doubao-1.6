#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <memory>
#include <string>
#include "models/user.h"
#include "models/user_repository.h"
#include "common/password_hasher.h"
#include "auth/token_manager.h"

class UserService {
public:
    UserService(
        std::shared_ptr<UserRepository> user_repository,
        std::shared_ptr<PasswordHasher> password_hasher,
        std::shared_ptr<TokenManager> token_manager)
        : user_repository_(user_repository)
        , password_hasher_(password_hasher)
        , token_manager_(token_manager) {
    }

    ~UserService() = default;

    // 禁止拷贝和移动
    UserService(const UserService&) = delete;
    UserService& operator=(const UserService&) = delete;
    UserService(UserService&&) = delete;
    UserService& operator=(UserService&&) = delete;

    // 用户注册
    bool RegisterUser(const std::string& username, const std::string& password, User& user);

    // 用户登录
    bool LoginUser(const std::string& username, const std::string& password, std::string& token);

    // 根据ID获取用户
    std::shared_ptr<User> GetUserById(int id);

    // 根据用户名获取用户
    std::shared_ptr<User> GetUserByUsername(const std::string& username);

    // 更新用户信息
    bool UpdateUser(const User& user);

    // 删除用户
    bool DeleteUser(int id);

    // 验证令牌
    bool ValidateToken(const std::string& token, int& user_id);

private:
    std::shared_ptr<UserRepository> user_repository_;
    std::shared_ptr<PasswordHasher> password_hasher_;
    std::shared_ptr<TokenManager> token_manager_;
};

#endif // USER_SERVICE_H
