#include "utils/TimeUtils.h"
#include <sstream>
#include <iomanip>

namespace utils {

std::string TimeUtils::getCurrentTimeString(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    return formatTime(now, format);
}

std::chrono::system_clock::time_point TimeUtils::parseTimeString(const std::string& timeStr, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, format.c_str());
    
    if (ss.fail()) {
        throw std::invalid_argument("无法解析时间字符串: " + timeStr);
    }
    
    #ifdef _WIN32
        return std::chrono::system_clock::from_time_t(_mktime64(&tm));
    #else
        return std::chrono::system_clock::from_time_t(mktime(&tm));
    #endif
}

std::string TimeUtils::formatTime(const std::chrono::system_clock::time_point& timePoint, const std::string& format) {
    auto time_t_time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = {};
    
    #ifdef _WIN32
        localtime_s(&tm, &time_t_time);
    #else
        localtime_r(&time_t_time, &tm);
    #endif
    
    std::stringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

int64_t TimeUtils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    return getTimestamp(now);
}

int64_t TimeUtils::getTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    auto duration = timePoint.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

int TimeUtils::getDaysBetween(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) {
    auto duration = std::chrono::duration_cast<std::chrono::hours>(end - start);
    return static_cast<int>(duration.count() / 24);
}

bool TimeUtils::isWithinLastDays(const std::chrono::system_clock::time_point& timePoint, int days) {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - timePoint);
    return duration.count() <= days * 24;
}

} // namespace utils