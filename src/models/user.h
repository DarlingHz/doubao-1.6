#ifndef USER_H
#define USER_H

#include <string>
#include <chrono>

class User {
public:
    User() = default;
    ~User() = default;

    // 获取用户ID
    int GetId() const;

    // 设置用户ID
    void SetId(int id);

    // 获取用户名
    std::string GetUsername() const;

    // 设置用户名
    void SetUsername(const std::string& username);

    // 获取密码哈希值
    std::string GetPasswordHash() const;

    // 设置密码哈希值
    void SetPasswordHash(const std::string& password_hash);

    // 获取用户创建时间
    std::chrono::system_clock::time_point GetCreatedAt() const;

    // 设置用户创建时间
    void SetCreatedAt(const std::chrono::system_clock::time_point& created_at);

    // 获取用户更新时间
    std::chrono::system_clock::time_point GetUpdatedAt() const;

    // 设置用户更新时间
    void SetUpdatedAt(const std::chrono::system_clock::time_point& updated_at);

    // 获取用户是否被删除
    bool IsDeleted() const;

    // 设置用户是否被删除
    void SetIsDeleted(bool is_deleted);

private:
    int id_ = 0;
    std::string username_;
    std::string password_hash_;
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point updated_at_;
    bool is_deleted_ = false;
};

#endif // USER_H
