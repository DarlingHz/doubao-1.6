#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <chrono>
#include <random>
#include <cstdint>

namespace api_quota {
namespace utils {

// 生成随机字符串用于API Key
extern std::string generate_random_string(size_t length = 32);

// 获取当前时间戳（秒）
extern uint64_t get_current_timestamp();

// 获取当天开始时间戳
extern uint64_t get_today_start_timestamp();

// 获取当前分钟开始时间戳
extern uint64_t get_current_minute_start_timestamp();

// 格式化时间戳为字符串
extern std::string format_timestamp(uint64_t timestamp);

// 解析日期字符串为时间戳（YYYY-MM-DD格式）
extern uint64_t parse_date_string(const std::string& date_str);

// 生成唯一ID
extern uint64_t generate_unique_id();

// 哈希函数，用于内存缓存键
extern uint64_t hash_string(const std::string& str);

} // namespace utils
} // namespace api_quota

#endif // UTILS_HPP