#ifndef STATISTICS_SERVICE_H
#define STATISTICS_SERVICE_H

#include "repository/WatchRecordRepository.h"
#include "repository/UserRepository.h"
#include "utils/Cache.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace service {

struct UserStatistics {
    int total_movies;           // 总共看过的影片数
    int total_watch_duration;   // 总观看时长（分钟）
    int recent_30days_count;    // 最近30天观影次数
    int recent_30days_duration; // 最近30天观看时长
    double average_rating;      // 平均评分
    std::vector<std::pair<std::string, int>> top_genres;  // 按类型分组的观看时长Top3
};

class StatisticsService {
public:
    StatisticsService(std::shared_ptr<repository::WatchRecordRepository> record_repository,
                     std::shared_ptr<repository::UserRepository> user_repository,
                     std::shared_ptr<utils::Cache> cache);
    
    // 获取用户统计信息
    UserStatistics getUserStatistics(int user_id);
    
    // 清除用户统计缓存
    void clearUserStatisticsCache(int user_id);
    
private:
    std::shared_ptr<repository::WatchRecordRepository> record_repository_;
    std::shared_ptr<repository::UserRepository> user_repository_;
    std::shared_ptr<utils::Cache> cache_;
    
    // 从缓存获取统计信息
    std::optional<UserStatistics> getStatisticsFromCache(int user_id);
    
    // 将统计信息存入缓存
    void saveStatisticsToCache(int user_id, const UserStatistics& stats);
};

} // namespace service

#endif // STATISTICS_SERVICE_H
