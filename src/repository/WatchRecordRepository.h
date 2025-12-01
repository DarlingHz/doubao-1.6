#ifndef WATCH_RECORD_REPOSITORY_H
#define WATCH_RECORD_REPOSITORY_H

#include "repository/Database.h"
#include "model/WatchRecord.h"
#include <vector>
#include <optional>
#include <ctime>

namespace repository {

class WatchRecordRepository {
public:
    explicit WatchRecordRepository(DatabasePtr db);
    
    // 创建观影记录
    std::optional<model::WatchRecord> create(const model::WatchRecord& record);
    
    // 根据ID查询观影记录
    std::optional<model::WatchRecord> findById(int id);
    
    // 根据用户ID分页查询观影记录
    std::vector<model::WatchRecord> findByUserId(int user_id, int page, int page_size,
                                                const std::time_t& start_time = 0,
                                                const std::time_t& end_time = 0);
    
    // 获取用户的统计信息
    int countByUserId(int user_id);
    int getTotalWatchDuration(int user_id);
    double getAverageRating(int user_id);
    
    // 获取用户按类型分组的观看时长
    std::vector<std::pair<std::string, int>> getGenresWatchDuration(int user_id, int limit = 3);
    
    // 获取最近30天的观影统计
    std::pair<int, int> getRecent30DaysStats(int user_id);
    
    // 获取用户观看过的影片ID列表
    std::vector<int> getWatchedMovieIds(int user_id);
    
private:
    DatabasePtr db_;
    
    // 从SQL结果构建观影记录对象
    static model::WatchRecord buildWatchRecordFromRow(char** row);
    
    // 静态回调函数
    static int findByIdCallback(void* data, int argc, char** argv, char**);
    static int findByUserIdCallback(void* data, int argc, char** argv, char**);
    static int getGenresWatchDurationCallback(void* data, int argc, char** argv, char**);
    static int getWatchedMovieIdsCallback(void* data, int argc, char** argv, char**);
    
    // 构建时间条件查询
    std::string buildTimeConditions(const std::time_t& start_time, const std::time_t& end_time);
};

} // namespace repository

#endif // WATCH_RECORD_REPOSITORY_H
