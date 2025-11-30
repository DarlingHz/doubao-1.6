#ifndef MEMORY_USER_REPOSITORY_H
#define MEMORY_USER_REPOSITORY_H

#include <unordered_map>
#include <mutex>
#include "user_repository.h"

class MemoryUserRepository : public UserRepository {
public:
    MemoryUserRepository() = default;
    ~MemoryUserRepository() override = default;

    // 创建用户
    bool CreateUser(User& user) override;

    // 根据ID获取用户
    std::shared_ptr<User> GetUserById(int id) override;

    // 根据用户名获取用户
    std::shared_ptr<User> GetUserByUsername(const std::string& username) override;

    // 更新用户信息
    bool UpdateUser(const User& user) override;

    // 删除用户（软删除）
    bool DeleteUser(int id) override;

    // 检查用户名是否存在
    bool IsUsernameExists(const std::string& username) override;

private:
    std::unordered_map<int, std::shared_ptr<User>> users_;
    std::unordered_map<std::string, int> username_to_id_;
    std::mutex mutex_;
    int next_id_ = 1;
};

#endif // MEMORY_USER_REPOSITORY_H
