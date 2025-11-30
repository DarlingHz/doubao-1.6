#include "user_service.h"
#include <chrono>

bool UserService::RegisterUser(const std::string& username, const std::string& password, User& user) {
    // 检查用户名是否已存在
    if (user_repository_->IsUsernameExists(username)) {
        return false;
    }

    // 对密码进行哈希处理
    std::string password_hash = password_hasher_->HashPassword(password);

    // 创建用户对象
    user.SetUsername(username);
    user.SetPasswordHash(password_hash);

    // 保存用户到数据库
    return user_repository_->CreateUser(user);
}

bool UserService::LoginUser(const std::string& username, const std::string& password, std::string& token) {
    // 根据用户名获取用户
    auto user = user_repository_->GetUserByUsername(username);
    if (!user) {
        return false;
    }

    // 验证密码是否正确
    if (!password_hasher_->VerifyPassword(password, user->GetPasswordHash())) {
        return false;
    }

    // 生成访问令牌，有效期为7天
    token = token_manager_->GenerateToken(user->GetId(), std::chrono::hours(24 * 7));

    return true;
}

std::shared_ptr<User> UserService::GetUserById(int id) {
    return user_repository_->GetUserById(id);
}

std::shared_ptr<User> UserService::GetUserByUsername(const std::string& username) {
    return user_repository_->GetUserByUsername(username);
}

bool UserService::UpdateUser(const User& user) {
    return user_repository_->UpdateUser(user);
}

bool UserService::DeleteUser(int id) {
    return user_repository_->DeleteUser(id);
}

bool UserService::ValidateToken(const std::string& token, int& user_id) {
    return token_manager_->ParseToken(token, user_id);
}
