#ifndef WATCH_RECORD_SERVICE_H
#define WATCH_RECORD_SERVICE_H

#include "repository/WatchRecordRepository.h"
#include "repository/UserRepository.h"
#include "repository/MovieRepository.h"
#include "model/WatchRecord.h"
#include <optional>
#include <vector>
#include <ctime>

namespace service {

struct WatchRecordPageResult {
    std::vector<model::WatchRecord> records;
    int total;
    int page;
    int page_size;
    int total_pages;
};

class WatchRecordService {
public:
    WatchRecordService(std::shared_ptr<repository::WatchRecordRepository> record_repository,
                      std::shared_ptr<repository::UserRepository> user_repository,
                      std::shared_ptr<repository::MovieRepository> movie_repository);
    
    // 创建观影记录
    std::optional<model::WatchRecord> createWatchRecord(int user_id, int movie_id, 
                                                      const std::time_t& start_time,
                                                      int watch_duration, bool is_completed,
                                                      int rating = 0, const std::string& comment = "");
    
    // 查询用户观影记录
    WatchRecordPageResult getUserWatchRecords(int user_id, int page, int page_size,
                                             const std::time_t& start_time = 0,
                                             const std::time_t& end_time = 0);
    
    // 验证观影记录信息
    bool validateWatchRecord(const model::WatchRecord& record);
    
private:
    std::shared_ptr<repository::WatchRecordRepository> record_repository_;
    std::shared_ptr<repository::UserRepository> user_repository_;
    std::shared_ptr<repository::MovieRepository> movie_repository_;
    
    // 验证评分
    bool validateRating(int rating);
    
    // 验证观看时长
    bool validateWatchDuration(int watch_duration, int movie_duration);
};

} // namespace service

#endif // WATCH_RECORD_SERVICE_H
