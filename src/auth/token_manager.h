#ifndef TOKEN_MANAGER_H
#define TOKEN_MANAGER_H

#include <string>
#include <chrono>

class TokenManager {
public:
    TokenManager() = default;
    virtual ~TokenManager() = default;

    // 禁止拷贝和移动
    TokenManager(const TokenManager&) = delete;
    TokenManager& operator=(const TokenManager&) = delete;
    TokenManager(TokenManager&&) = delete;
    TokenManager& operator=(TokenManager&&) = delete;

    // 生成访问令牌
    virtual std::string GenerateToken(int user_id, const std::chrono::duration<int64_t>& expires_in) = 0;

    // 解析访问令牌
    virtual bool ParseToken(const std::string& token, int& user_id) = 0;

    // 验证令牌是否有效
    virtual bool IsTokenValid(const std::string& token) = 0;
};

#endif // TOKEN_MANAGER_H
