#ifndef PASSWORD_HASHER_H
#define PASSWORD_HASHER_H

#include <string>

class PasswordHasher {
public:
    PasswordHasher() = default;
    virtual ~PasswordHasher() = default;

    // 禁止拷贝和移动
    PasswordHasher(const PasswordHasher&) = delete;
    PasswordHasher& operator=(const PasswordHasher&) = delete;
    PasswordHasher(PasswordHasher&&) = delete;
    PasswordHasher& operator=(PasswordHasher&&) = delete;

    // 对密码进行哈希处理
    virtual std::string HashPassword(const std::string& password) = 0;

    // 验证密码是否与哈希值匹配
    virtual bool VerifyPassword(const std::string& password, const std::string& hash) = 0;
};

#endif // PASSWORD_HASHER_H
