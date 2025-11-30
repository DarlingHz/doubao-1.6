#ifndef SIMPLE_PASSWORD_HASHER_H
#define SIMPLE_PASSWORD_HASHER_H

#include "password_hasher.h"
#include <random>

class SimplePasswordHasher : public PasswordHasher {
public:
    SimplePasswordHasher();
    ~SimplePasswordHasher() override = default;

    // 对密码进行哈希处理
    std::string HashPassword(const std::string& password) override;

    // 验证密码是否与哈希值匹配
    bool VerifyPassword(const std::string& password, const std::string& hashed_password) override;

private:
    // 生成随机盐值
    std::string GenerateSalt();

    // 计算字符串的哈希值
    std::string ComputeHash(const std::string& input);

    // 将字符串转换为十六进制字符串
    std::string ToHexString(const std::string& input);

private:
    std::mt19937 salt_generator_;
};

#endif // SIMPLE_PASSWORD_HASHER_H
