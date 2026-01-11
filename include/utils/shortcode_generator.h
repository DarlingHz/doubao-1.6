#pragma once

#include <string>
#include <random>
#include <mutex>

namespace utils {

class ShortCodeGenerator {
public:
    ShortCodeGenerator(int defaultLength = 6) : defaultLength_(defaultLength) {
        // 初始化字符集：大小写字母 + 数字
        charset_ = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        // 初始化随机数生成器
        std::random_device rd;
        gen_.seed(rd());
        dist_ = std::uniform_int_distribution<>(0, charset_.size() - 1);
    }

    // 生成随机短码
    std::string generateRandomCode(int length = 0) {
        if (length <= 0) {
            length = defaultLength_;
        }

        std::string code;
        code.reserve(length);

        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < length; ++i) {
            code += charset_[dist_(gen_)];
        }

        return code;
    }

    // 验证短码是否有效
    bool isValidShortCode(const std::string& code, int minLength = 4, int maxLength = 20) {
        if (code.empty() || code.length() < static_cast<size_t>(minLength) || code.length() > static_cast<size_t>(maxLength)) {
            return false;
        }

        // 验证字符是否都在允许的字符集中
        for (char c : code) {
            if (charset_.find(c) == std::string::npos) {
                return false;
            }
        }

        return true;
    }

    // 设置默认短码长度
    void setDefaultLength(int length) {
        if (length > 0) {
            defaultLength_ = length;
        }
    }

    // 获取默认短码长度
    int getDefaultLength() const {
        return defaultLength_;
    }

private:
    int defaultLength_;  // 默认短码长度
    std::string charset_;  // 字符集
    std::mt19937 gen_;  // 随机数生成器
    std::uniform_int_distribution<> dist_;  // 均匀分布
    std::mutex mutex_;  // 互斥锁，保证线程安全
};

} // namespace utils
