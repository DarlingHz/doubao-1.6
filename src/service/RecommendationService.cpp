#include "service/RecommendationService.h"
#include <stdexcept>
#include <unordered_set>

namespace service {

RecommendationService::RecommendationService(std::shared_ptr<repository::MovieRepository> movie_repository,
                                           std::shared_ptr<repository::WatchRecordRepository> record_repository,
                                           std::shared_ptr<repository::UserRepository> user_repository)
    : movie_repository_(movie_repository),
      record_repository_(record_repository),
      user_repository_(user_repository) {}

std::vector<model::Movie> RecommendationService::getRecommendations(int user_id, int limit) {
    // 验证用户是否存在
    if (!user_repository_->findById(user_id).has_value()) {
        throw std::invalid_argument("用户不存在");
    }
    
    std::vector<model::Movie> recommendations;
    
    // 1. 优先基于用户喜欢的类型推荐
    auto genre_recommendations = recommendByFavoriteGenres(user_id, limit);
    recommendations.insert(recommendations.end(), genre_recommendations.begin(), genre_recommendations.end());
    
    // 2. 如果推荐数量不足，补充推荐热门未看影片
    if (recommendations.size() < limit) {
        int remaining = limit - recommendations.size();
        auto popular_recommendations = recommendPopularUnwatched(user_id, remaining);
        
        // 避免重复推荐
        std::unordered_set<int> recommended_ids;
        for (const auto& movie : recommendations) {
            recommended_ids.insert(movie.getId());
        }
        
        for (const auto& movie : popular_recommendations) {
            if (recommended_ids.find(movie.getId()) == recommended_ids.end()) {
                recommendations.push_back(movie);
                if (recommendations.size() >= limit) {
                    break;
                }
            }
        }
    }
    
    return recommendations;
}

std::vector<model::Movie> RecommendationService::recommendByFavoriteGenres(int user_id, int limit) {
    // 获取用户最喜欢的类型
    auto favorite_genres = getUserFavoriteGenres(user_id, 3);
    if (favorite_genres.empty()) {
        return {};
    }
    
    std::vector<model::Movie> recommendations;
    std::unordered_set<int> recommended_ids;
    
    // 获取用户已观看的影片ID
    auto watched_ids = record_repository_->getWatchedMovieIds(user_id);
    std::unordered_set<int> watched_set(watched_ids.begin(), watched_ids.end());
    
    // 按类型优先级推荐
    for (const auto& genre : favorite_genres) {
        // 查询该类型的影片
        auto genre_movies = movie_repository_->findAll(1, 20, "", genre);
        
        for (const auto& movie : genre_movies) {
            // 跳过已观看的影片
            if (watched_set.find(movie.getId()) == watched_set.end() &&
                recommended_ids.find(movie.getId()) == recommended_ids.end()) {
                recommendations.push_back(movie);
                recommended_ids.insert(movie.getId());
                
                if (recommendations.size() >= limit) {
                    return recommendations;
                }
            }
        }
    }
    
    return recommendations;
}

std::vector<std::string> RecommendationService::getUserFavoriteGenres(int user_id, int limit) {
    std::vector<std::string> favorite_genres;
    
    // 获取按观看时长排序的类型列表
    auto genre_stats = record_repository_->getGenresWatchDuration(user_id, limit);
    
    // 正确访问pair中的genre字符串
    for (const auto& genre_stat : genre_stats) {
        favorite_genres.push_back(genre_stat.first);
    }
    
    return favorite_genres;
}

std::vector<model::Movie> RecommendationService::recommendPopularUnwatched(int user_id, int limit) {
    // 简单实现：直接获取用户未观看过的影片
    // 实际项目中可以基于更复杂的热门度算法
    return movie_repository_->findUnwatchedByUserId(user_id, limit);
}

} // namespace service
