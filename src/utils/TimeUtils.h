#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <string>
#include <chrono>
#include <ctime>

namespace utils {

class TimeUtils {
public:
    static std::string getCurrentTimeString(const std::string& format = "%Y-%m-%d %H:%M:%S");
    static std::chrono::system_clock::time_point parseTimeString(const std::string& timeStr, const std::string& format = "%Y-%m-%d %H:%M:%S");
    static std::string formatTime(const std::chrono::system_clock::time_point& timePoint, const std::string& format = "%Y-%m-%d %H:%M:%S");
    static int64_t getCurrentTimestamp();
    static int64_t getTimestamp(const std::chrono::system_clock::time_point& timePoint);
    static int getDaysBetween(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);
    static bool isWithinLastDays(const std::chrono::system_clock::time_point& timePoint, int days);
};

} // namespace utils

#endif // TIMEUTILS_H