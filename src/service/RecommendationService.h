#ifndef RECOMMENDATION_SERVICE_H
#define RECOMMENDATION_SERVICE_H

#include "repository/MovieRepository.h"
#include "repository/WatchRecordRepository.h"
#include "repository/UserRepository.h"
#include "model/Movie.h"
#include <vector>
#include <memory>

namespace service {

class RecommendationService {
public:
    RecommendationService(std::shared_ptr<repository::MovieRepository> movie_repository,
                         std::shared_ptr<repository::WatchRecordRepository> record_repository,
                         std::shared_ptr<repository::UserRepository> user_repository);
    
    // 获取用户推荐影片列表
    std::vector<model::Movie> getRecommendations(int user_id, int limit = 10);
    
private:
    std::shared_ptr<repository::MovieRepository> movie_repository_;
    std::shared_ptr<repository::WatchRecordRepository> record_repository_;
    std::shared_ptr<repository::UserRepository> user_repository_;
    
    // 基于用户最常看的类型推荐
    std::vector<model::Movie> recommendByFavoriteGenres(int user_id, int limit);
    
    // 获取用户最常看的类型列表
    std::vector<std::string> getUserFavoriteGenres(int user_id, int limit = 3);
    
    // 推荐热门未看影片
    std::vector<model::Movie> recommendPopularUnwatched(int user_id, int limit);
};

} // namespace service

#endif // RECOMMENDATION_SERVICE_H
