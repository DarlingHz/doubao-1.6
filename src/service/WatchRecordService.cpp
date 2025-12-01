#include "service/WatchRecordService.h"
#include <stdexcept>

namespace service {

WatchRecordService::WatchRecordService(std::shared_ptr<repository::WatchRecordRepository> record_repository,
                                      std::shared_ptr<repository::UserRepository> user_repository,
                                      std::shared_ptr<repository::MovieRepository> movie_repository)
    : record_repository_(record_repository),
      user_repository_(user_repository),
      movie_repository_(movie_repository) {}

std::optional<model::WatchRecord> WatchRecordService::createWatchRecord(int user_id, int movie_id, 
                                                                      const std::time_t& start_time,
                                                                      int watch_duration, bool is_completed,
                                                                      int rating, const std::string& comment) {
    // 验证用户是否存在
    if (!user_repository_->findById(user_id).has_value()) {
        throw std::invalid_argument("用户不存在");
    }
    
    // 验证影片是否存在
    auto movie_opt = movie_repository_->findById(movie_id);
    if (!movie_opt.has_value()) {
        throw std::invalid_argument("影片不存在");
    }
    
    // 创建记录
    model::WatchRecord record;
    record.setUserId(user_id);
    record.setMovieId(movie_id);
    record.setStartTime(start_time);
    record.setWatchDuration(watch_duration);
    record.setCompleted(is_completed);
    record.setRating(rating);
    record.setComment(comment);
    
    // 验证记录信息
    if (!validateWatchRecord(record)) {
        throw std::invalid_argument("观影记录信息验证失败");
    }
    
    // 验证观看时长不超过影片总时长
    if (!validateWatchDuration(watch_duration, movie_opt->getDuration())) {
        throw std::invalid_argument("观看时长不能超过影片总时长");
    }
    
    return record_repository_->create(record);
}

WatchRecordPageResult WatchRecordService::getUserWatchRecords(int user_id, int page, int page_size,
                                                             const std::time_t& start_time,
                                                             const std::time_t& end_time) {
    // 验证用户是否存在
    if (!user_repository_->findById(user_id).has_value()) {
        throw std::invalid_argument("用户不存在");
    }
    
    // 参数验证
    if (page < 1) {
        page = 1;
    }
    if (page_size < 1 || page_size > 100) {
        page_size = 20;
    }
    
    // 查询数据
    std::vector<model::WatchRecord> records = record_repository_->findByUserId(user_id, page, page_size, start_time, end_time);
    int total = record_repository_->countByUserId(user_id);
    int total_pages = (total + page_size - 1) / page_size;
    
    return {records, total, page, page_size, total_pages};
}

bool WatchRecordService::validateWatchRecord(const model::WatchRecord& record) {
    // 验证用户ID和影片ID
    if (record.getUserId() <= 0 || record.getMovieId() <= 0) {
        return false;
    }
    
    // 验证开始时间
    if (record.getStartTime() <= 0 || record.getStartTime() > std::time(nullptr)) {
        return false;
    }
    
    // 验证观看时长
    if (record.getWatchDuration() <= 0) {
        return false;
    }
    
    // 验证评分
    if (!validateRating(record.getRating())) {
        return false;
    }
    
    // 验证备注长度
    if (record.getComment().length() > 500) {
        return false;
    }
    
    return true;
}

bool WatchRecordService::validateRating(int rating) {
    return rating == 0 || (rating >= 1 && rating <= 10);
}

bool WatchRecordService::validateWatchDuration(int watch_duration, int movie_duration) {
    // 允许观看时长略大于影片时长（例如重复观看的情况）
    return watch_duration <= movie_duration * 2;
}

} // namespace service
