#ifndef SIMPLE_TOKEN_MANAGER_H
#define SIMPLE_TOKEN_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "token_manager.h"

class SimpleTokenManager : public TokenManager {
public:
    SimpleTokenManager() = default;
    explicit SimpleTokenManager(const std::chrono::duration<int64_t>& default_expires_in)
        : default_expires_in_(default_expires_in) {}
    ~SimpleTokenManager() override = default;

    // 生成访问令牌
    std::string GenerateToken(int user_id, const std::chrono::duration<int64_t>& expires_in) override;

    // 解析访问令牌
    bool ParseToken(const std::string& token, int& user_id) override;

    // 验证令牌是否有效
    bool IsTokenValid(const std::string& token) override;

private:
    // Base64编码
    std::string Base64Encode(const std::string& input) const;

    // Base64解码
    std::string Base64Decode(const std::string& input) const;

    // 检查字符是否为Base64字符
    bool is_base64(unsigned char c) const;

    // 清理过期的令牌
    void CleanExpiredTokens();

private:
    struct TokenInfo {
        int user_id;
        std::chrono::system_clock::time_point expires_at;
    };

    std::unordered_map<std::string, TokenInfo> tokens_;
    std::mutex mutex_;
    std::chrono::duration<int64_t> default_expires_in_ = std::chrono::hours(24);
};

#endif // SIMPLE_TOKEN_MANAGER_H
