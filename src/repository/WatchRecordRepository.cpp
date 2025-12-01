#include "repository/WatchRecordRepository.h"
#include <sstream>
#include <iomanip>

namespace repository {

// 静态回调函数实现
int WatchRecordRepository::findByIdCallback(void* data, int argc, char** argv, char**) {
    auto* result = static_cast<std::optional<model::WatchRecord>*>(data);
    if (argc >= 10 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] && argv[5]) {
        *result = WatchRecordRepository::buildWatchRecordFromRow(argv);
    }
    return 0;
}

int WatchRecordRepository::findByUserIdCallback(void* data, int argc, char** argv, char**) {
    auto* records = static_cast<std::vector<model::WatchRecord>*>(data);
    if (argc >= 10 && argv[0] && argv[1] && argv[2] && argv[3] && argv[4] && argv[5]) {
        records->push_back(WatchRecordRepository::buildWatchRecordFromRow(argv));
    }
    return 0;
}

int WatchRecordRepository::getGenresWatchDurationCallback(void* data, int argc, char** argv, char**) {
    auto* genre_stats = static_cast<std::vector<std::pair<std::string, int>>*>(data);
    if (argc >= 2 && argv[0] && argv[1]) {
        genre_stats->emplace_back(argv[0], std::stoi(argv[1]));
    }
    return 0;
}

int WatchRecordRepository::getWatchedMovieIdsCallback(void* data, int argc, char** argv, char**) {
    auto* movie_ids = static_cast<std::vector<int>*>(data);
    if (argc >= 1 && argv[0]) {
        movie_ids->push_back(std::stoi(argv[0]));
    }
    return 0;
}

WatchRecordRepository::WatchRecordRepository(DatabasePtr db) : db_(db) {}

std::optional<model::WatchRecord> WatchRecordRepository::create(const model::WatchRecord& record) {
    std::ostringstream sql;
    sql << "INSERT INTO watch_records (user_id, movie_id, start_time, watch_duration, is_completed";
    
    // 添加可选字段
    bool has_rating = record.getRating() > 0;
    bool has_comment = !record.getComment().empty();
    
    if (has_rating) {
        sql << ", rating";
    }
    if (has_comment) {
        sql << ", comment";
    }
    
    sql << ") VALUES (" << record.getUserId() << ", " << record.getMovieId() << ", '" 
        << std::put_time(std::localtime(&record.getStartTime()), "%Y-%m-%d %H:%M:%S") 
        << "', " << record.getWatchDuration() << ", " << (record.isCompleted() ? 1 : 0);
    
    if (has_rating) {
        sql << ", " << record.getRating();
    }
    if (has_comment) {
        sql << ", '" << record.getComment() << "'";
    }
    
    sql << ");";
    
    if (db_->execute(sql.str())) {
        int id = static_cast<int>(db_->getLastInsertId());
        return findById(id);
    }
    
    return std::nullopt;
}

std::optional<model::WatchRecord> WatchRecordRepository::findById(int id) {
    std::ostringstream sql;
    sql << "SELECT id, user_id, movie_id, start_time, watch_duration, is_completed, rating, comment, created_at, updated_at FROM watch_records WHERE id = " << id << ";";
    
    std::optional<model::WatchRecord> result;
    
    db_->query(sql.str(), findByIdCallback, &result);
    return result;
}

std::vector<model::WatchRecord> WatchRecordRepository::findByUserId(int user_id, int page, int page_size,
                                                                   const std::time_t& start_time, const std::time_t& end_time) {
    std::vector<model::WatchRecord> records;
    
    std::string time_conditions = buildTimeConditions(start_time, end_time);
    int offset = (page - 1) * page_size;
    
    std::ostringstream sql;
    sql << "SELECT id, user_id, movie_id, start_time, watch_duration, is_completed, rating, comment, created_at, updated_at " 
        << "FROM watch_records WHERE user_id = " << user_id << time_conditions 
        << " ORDER BY start_time DESC LIMIT " << page_size << " OFFSET " << offset << ";";
    
    db_->query(sql.str(), findByUserIdCallback, &records);
    return records;
}

int WatchRecordRepository::countByUserId(int user_id) {
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM watch_records WHERE user_id = " << user_id << ";";
    
    std::string result = db_->queryScalar(sql.str());
    return result.empty() ? 0 : std::stoi(result);
}

int WatchRecordRepository::getTotalWatchDuration(int user_id) {
    std::ostringstream sql;
    sql << "SELECT SUM(watch_duration) FROM watch_records WHERE user_id = " << user_id << ";";
    
    std::string result = db_->queryScalar(sql.str());
    return result.empty() ? 0 : std::stoi(result);
}

double WatchRecordRepository::getAverageRating(int user_id) {
    std::ostringstream sql;
    sql << "SELECT AVG(rating) FROM watch_records WHERE user_id = " << user_id << " AND rating > 0;";
    
    std::string result = db_->queryScalar(sql.str());
    return result.empty() ? 0.0 : std::stod(result);
}

std::vector<std::pair<std::string, int>> WatchRecordRepository::getGenresWatchDuration(int user_id, int limit) {
    std::vector<std::pair<std::string, int>> genre_stats;
    
    // 这个查询比较复杂，需要使用临时表来处理JSON数组
    std::ostringstream sql;
    sql << "WITH genre_data AS (" 
        << "  SELECT w.watch_duration, json_each.value as genre " 
        << "  FROM watch_records w " 
        << "  JOIN movies m ON w.movie_id = m.id " 
        << "  JOIN json_each(json_extract(m.genres, '$')) " 
        << "  WHERE w.user_id = " << user_id 
        << ") " 
        << "SELECT genre, SUM(watch_duration) as total_duration " 
        << "FROM genre_data " 
        << "GROUP BY genre " 
        << "ORDER BY total_duration DESC " 
        << "LIMIT " << limit << ";";
    
    db_->query(sql.str(), getGenresWatchDurationCallback, &genre_stats);
    return genre_stats;
}

std::pair<int, int> WatchRecordRepository::getRecent30DaysStats(int user_id) {
    std::time_t now = std::time(nullptr);
    std::time_t thirty_days_ago = now - (30 * 24 * 60 * 60);
    
    std::ostringstream sql_count;
    sql_count << "SELECT COUNT(*) FROM watch_records WHERE user_id = " << user_id 
              << " AND start_time >= datetime('now', '-30 days');";
    
    std::ostringstream sql_duration;
    sql_duration << "SELECT SUM(watch_duration) FROM watch_records WHERE user_id = " << user_id 
                 << " AND start_time >= datetime('now', '-30 days');";
    
    std::string count_str = db_->queryScalar(sql_count.str());
    std::string duration_str = db_->queryScalar(sql_duration.str());
    
    int count = count_str.empty() ? 0 : std::stoi(count_str);
    int duration = duration_str.empty() ? 0 : std::stoi(duration_str);
    
    return {count, duration};
}

std::vector<int> WatchRecordRepository::getWatchedMovieIds(int user_id) {
    std::vector<int> movie_ids;
    
    std::ostringstream sql;
    sql << "SELECT DISTINCT movie_id FROM watch_records WHERE user_id = " << user_id << ";";
    
    db_->query(sql.str(), getWatchedMovieIdsCallback, &movie_ids);
    return movie_ids;
}

model::WatchRecord WatchRecordRepository::buildWatchRecordFromRow(char** row) {
    int id = std::stoi(row[0]);
    int user_id = std::stoi(row[1]);
    int movie_id = std::stoi(row[2]);
    
    // 解析时间字符串
    std::tm tm = {};
    std::istringstream start_time_ss(row[3]);
    start_time_ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::time_t start_time = std::mktime(&tm);
    
    int watch_duration = std::stoi(row[4]);
    bool is_completed = std::stoi(row[5]) != 0;
    
    int rating = 0;
    if (row[6]) {
        rating = std::stoi(row[6]);
    }
    
    std::string comment;
    if (row[7]) {
        comment = row[7];
    }
    
    std::tm tm2 = {};
    std::istringstream created_at_ss(row[8]);
    created_at_ss >> std::get_time(&tm2, "%Y-%m-%d %H:%M:%S");
    std::time_t created_at = std::mktime(&tm2);
    
    std::tm tm3 = {};
    std::istringstream updated_at_ss(row[9]);
    updated_at_ss >> std::get_time(&tm3, "%Y-%m-%d %H:%M:%S");
    std::time_t updated_at = std::mktime(&tm3);
    
    return model::WatchRecord(id, user_id, movie_id, start_time, watch_duration, 
                             is_completed, rating, comment, created_at, updated_at);
}

std::string WatchRecordRepository::buildTimeConditions(const std::time_t& start_time, const std::time_t& end_time) {
    std::ostringstream conditions;
    
    if (start_time > 0) {
        conditions << " AND start_time >= '" 
                  << std::put_time(std::localtime(&start_time), "%Y-%m-%d %H:%M:%S") 
                  << "'";
    }
    
    if (end_time > 0) {
        conditions << " AND start_time <= '" 
                  << std::put_time(std::localtime(&end_time), "%Y-%m-%d %H:%M:%S") 
                  << "'";
    }
    
    return conditions.str();
}

} // namespace repository
