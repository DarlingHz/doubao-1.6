#include "utils/utils.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

namespace api_quota {
namespace utils {

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);
static std::mutex id_mutex;
static uint64_t last_id = 0;

std::string generate_random_string(size_t length) {
    std::stringstream ss;
    ss << std::hex;
    
    for (size_t i = 0; i < length / 2; ++i) {
        ss << std::setw(1) << std::setfill('0');
        ss << dis(gen);
    }
    
    return ss.str();
}

uint64_t get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

uint64_t get_today_start_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&time);
    local_tm.tm_hour = 0;
    local_tm.tm_min = 0;
    local_tm.tm_sec = 0;
    auto today_start = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
    auto duration = today_start.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

uint64_t get_current_minute_start_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&time);
    local_tm.tm_sec = 0;
    auto minute_start = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
    auto duration = minute_start.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

std::string format_timestamp(uint64_t timestamp) {
    auto time_point = std::chrono::system_clock::time_point(std::chrono::seconds(timestamp));
    auto time = std::chrono::system_clock::to_time_t(time_point);
    std::tm local_tm = *std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

uint64_t parse_date_string(const std::string& date_str) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        return 0;
    }
    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto duration = time_point.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

uint64_t generate_unique_id() {
    std::lock_guard<std::mutex> lock(id_mutex);
    uint64_t current = get_current_timestamp();
    if (current > last_id) {
        last_id = current;
        return current;
    }
    return ++last_id;
}

uint64_t hash_string(const std::string& str) {
    uint64_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return hash;
}

} // namespace utils
} // namespace api_quota