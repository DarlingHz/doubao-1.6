#include "service/StatisticsService.h"
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace service {

StatisticsService::StatisticsService(std::shared_ptr<repository::WatchRecordRepository> record_repository,
                                   std::shared_ptr<repository::UserRepository> user_repository,
                                   std::shared_ptr<utils::Cache> cache)
    : record_repository_(record_repository),
      user_repository_(user_repository),
      cache_(cache) {}

UserStatistics StatisticsService::getUserStatistics(int user_id) {
    // 验证用户是否存在
    if (!user_repository_->findById(user_id).has_value()) {
        throw std::invalid_argument("用户不存在");
    }
    
    // 尝试从缓存获取
    auto cached_stats = getStatisticsFromCache(user_id);
    if (cached_stats.has_value()) {
        return cached_stats.value();
    }
    
    // 计算统计信息
    UserStatistics stats;
    
    // 获取观看过的影片数（去重）
    auto watched_movie_ids = record_repository_->getWatchedMovieIds(user_id);
    stats.total_movies = watched_movie_ids.size();
    
    // 获取总观看时长
    stats.total_watch_duration = record_repository_->getTotalWatchDuration(user_id);
    
    // 获取最近30天统计
    auto recent_stats = record_repository_->getRecent30DaysStats(user_id);
    stats.recent_30days_count = recent_stats.first;
    stats.recent_30days_duration = recent_stats.second;
    
    // 获取平均评分
    stats.average_rating = record_repository_->getAverageRating(user_id);
    
    // 获取类型Top3
    stats.top_genres = record_repository_->getGenresWatchDuration(user_id, 3);
    
    // 保存到缓存
    saveStatisticsToCache(user_id, stats);
    
    return stats;
}

void StatisticsService::clearUserStatisticsCache(int user_id) {
    std::ostringstream cache_key;
    cache_key << "user_stats_" << user_id;
    cache_->remove(cache_key.str());
}

std::optional<UserStatistics> StatisticsService::getStatisticsFromCache(int user_id) {
    std::ostringstream cache_key;
    cache_key << "user_stats_" << user_id;
    
    auto cached_data = cache_->get(cache_key.str());
    if (cached_data.has_value()) {
        try {
            // 简单的键值对解析格式: key1=value1;key2=value2;...
            const std::string& data = cached_data.value();
            UserStatistics stats;
            
            size_t pos = 0;
            while (pos < data.size()) {
                size_t end_pos = data.find(';', pos);
                if (end_pos == std::string::npos) {
                    end_pos = data.size();
                }
                
                std::string key_value = data.substr(pos, end_pos - pos);
                size_t eq_pos = key_value.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = key_value.substr(0, eq_pos);
                    std::string value = key_value.substr(eq_pos + 1);
                    
                    if (key == "total_movies") stats.total_movies = std::stoi(value);
                    else if (key == "total_watch_duration") stats.total_watch_duration = std::stoi(value);
                    else if (key == "recent_30days_count") stats.recent_30days_count = std::stoi(value);
                    else if (key == "recent_30days_duration") stats.recent_30days_duration = std::stoi(value);
                    else if (key == "average_rating") stats.average_rating = std::stod(value);
                    // top_genres暂时简单处理，不存储在缓存中
                }
                
                pos = end_pos + 1;
            }
            
            return stats;
        } catch (...) {
            // 解析失败，返回空
        }
    }
    
    return std::nullopt;
}

void StatisticsService::saveStatisticsToCache(int user_id, const UserStatistics& stats) {
    std::ostringstream cache_key;
    cache_key << "user_stats_" << user_id;
    
    // 简单的键值对格式: key1=value1;key2=value2;...
    std::ostringstream data;
    data << "total_movies=" << stats.total_movies << ';'
         << "total_watch_duration=" << stats.total_watch_duration << ';'
         << "recent_30days_count=" << stats.recent_30days_count << ';'
         << "recent_30days_duration=" << stats.recent_30days_duration << ';'
         << "average_rating=" << stats.average_rating;
    
    cache_->set(cache_key.str(), data.str());
}

} // namespace service
