#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace utils {

class TimeUtils {
public:
    // 获取当前时间戳（秒）
    static uint64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 获取当前时间戳（毫秒）
    static uint64_t getCurrentTimestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 将时间戳格式化为字符串
    static std::string formatTimestamp(uint64_t timestamp) {
        auto timePoint = std::chrono::system_clock::time_point(
            std::chrono::seconds(timestamp));
        
        std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
        std::tm* localTime = std::localtime(&timeT);
        
        std::stringstream ss;
        ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // 计算过期时间戳
    static uint64_t calculateExpireTime(uint64_t currentTimestamp, int expireSeconds) {
        if (expireSeconds <= 0) {
            return 0;  // 0 表示永不过期
        }
        return currentTimestamp + expireSeconds;
    }

    // 检查是否已过期
    static bool isExpired(uint64_t expireTimestamp) {
        if (expireTimestamp == 0) {
            return false;  // 永不过期
        }
        return getCurrentTimestamp() > expireTimestamp;
    }

    // 获取当前时间的ISO 8601格式
    static std::string getCurrentISO8601Time() {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&now_time);
        
        std::stringstream ss;
        ss << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S");
        
        // 添加时区信息
        char timezone[6];
        std::strftime(timezone, sizeof(timezone), "%z", &local_tm);
        ss << timezone;
        
        return ss.str();
    }
};

} // namespace utils
