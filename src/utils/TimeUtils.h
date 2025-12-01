#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <ctime>

class TimeUtils {
public:
    // 获取当前时间戳（毫秒）
    static long long getCurrentTimestamp();
    
    // 获取当前时间戳（秒）
    static time_t getCurrentTimeSeconds();
    
    // 格式化时间戳为字符串
    static std::string formatTimestamp(long long timestamp, const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    // 格式化当前时间为字符串
    static std::string formatCurrentTime(const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    // 获取指定月份的开始时间戳
    static time_t getMonthStart(int year, int month);
    
    // 获取指定月份的结束时间戳
    static time_t getMonthEnd(int year, int month);
    
    // 解析日期字符串为时间戳
    static time_t parseDate(const std::string& dateStr, const std::string& format = "%Y-%m-%d");
    
    // 判断是否为闰年
    static bool isLeapYear(int year);
    
    // 获取两个时间戳之间的天数差
    static int getDaysDiff(time_t start, time_t end);
};

#endif // TIME_UTILS_H