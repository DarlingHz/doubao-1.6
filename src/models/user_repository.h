#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <memory>
#include <vector>
#include "user.h"

class UserRepository {
public:
    UserRepository() = default;
    virtual ~UserRepository() = default;

    // 禁止拷贝和移动
    UserRepository(const UserRepository&) = delete;
    UserRepository& operator=(const UserRepository&) = delete;
    UserRepository(UserRepository&&) = delete;
    UserRepository& operator=(UserRepository&&) = delete;

    // 创建用户
    virtual bool CreateUser(User& user) = 0;

    // 根据ID获取用户
    virtual std::shared_ptr<User> GetUserById(int id) = 0;

    // 根据用户名获取用户
    virtual std::shared_ptr<User> GetUserByUsername(const std::string& username) = 0;

    // 更新用户信息
    virtual bool UpdateUser(const User& user) = 0;

    // 删除用户（软删除）
    virtual bool DeleteUser(int id) = 0;

    // 检查用户名是否存在
    virtual bool IsUsernameExists(const std::string& username) = 0;
};

#endif // USER_REPOSITORY_H
