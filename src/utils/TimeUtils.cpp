#include "TimeUtils.h"
#include <chrono>
#include <iomanip>
#include <sstream>

long long TimeUtils::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

time_t TimeUtils::getCurrentTimeSeconds() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string TimeUtils::formatTimestamp(long long timestamp, const std::string& format) {
    auto timePoint = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(timestamp));
    time_t time = std::chrono::system_clock::to_time_t(timePoint);
    
    std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &time);
    #else
        localtime_r(&time, &tm);
    #endif
    
    std::stringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

std::string TimeUtils::formatCurrentTime(const std::string& format) {
    return formatTimestamp(getCurrentTimestamp(), format);
}

time_t TimeUtils::getMonthStart(int year, int month) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return mktime(&tm);
}

time_t TimeUtils::getMonthEnd(int year, int month) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month;
    tm.tm_mday = 0;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    return mktime(&tm);
}

time_t TimeUtils::parseDate(const std::string& dateStr, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, format.c_str());
    return mktime(&tm);
}

bool TimeUtils::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int TimeUtils::getDaysDiff(time_t start, time_t end) {
    const int SECONDS_PER_DAY = 24 * 60 * 60;
    return static_cast<int>((end - start) / SECONDS_PER_DAY);
}