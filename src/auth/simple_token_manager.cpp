#include "simple_token_manager.h"
#include <random>
#include <chrono>
#include <sstream>

std::string SimpleTokenManager::GenerateToken(int user_id, const std::chrono::duration<int64_t>& expires_in) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 清理过期的令牌
    CleanExpiredTokens();

    // 生成随机字符串作为令牌
    std::string token;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 32; ++i) {
        token += static_cast<char>(dis(gen));
    }

    token = Base64Encode(token);

    // 保存令牌信息
    TokenInfo token_info;
    token_info.user_id = user_id;
    token_info.expires_at = std::chrono::system_clock::now() + expires_in;

    tokens_[token] = token_info;

    return token;
}

bool SimpleTokenManager::ParseToken(const std::string& token, int& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 清理过期的令牌
    CleanExpiredTokens();

    auto it = tokens_.find(token);
    if (it == tokens_.end()) {
        return false;
    }

    user_id = it->second.user_id;
    return true;
}

bool SimpleTokenManager::IsTokenValid(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 清理过期的令牌
    CleanExpiredTokens();

    return tokens_.find(token) != tokens_.end();
}

std::string SimpleTokenManager::Base64Encode(const std::string& input) const {
    const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.c_str());
    int in_len = input.size();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i <4) ; i++) {
                output += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            output += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            output += '=';
        }
    }

    return output;
}

std::string SimpleTokenManager::Base64Decode(const std::string& input) const {
    const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4];
    unsigned char char_array_3[3];

    int input_len = input.size();
    if (input_len == 0) {
        return output;
    }

    while (input_len-- && (input[in_] != '=') && is_base64(input[in_])) {
        char_array_4[i++] = input[in_]; in_++;
        if (i == 4) {
            for (i = 0; i <4; i++) {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                output += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++) {
            char_array_4[j] = 0;
        }

        for (j = 0; j <4; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) {
            output += char_array_3[j];
        }
    }

    return output;
}

void SimpleTokenManager::CleanExpiredTokens() {
    auto now = std::chrono::system_clock::now();
    auto it = tokens_.begin();

    while (it != tokens_.end()) {
        if (it->second.expires_at <= now) {
            it = tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

bool SimpleTokenManager::is_base64(unsigned char c) const {
    return (isalnum(c) || (c == '+') || (c == '/'));
}
