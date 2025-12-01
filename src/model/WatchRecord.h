#ifndef WATCH_RECORD_H
#define WATCH_RECORD_H

#include <string>
#include <ctime>
#include <map>

namespace model {

class WatchRecord {
public:
    WatchRecord() = default;
    WatchRecord(int id, int user_id, int movie_id, const std::time_t& start_time,
                int watch_duration, bool is_completed, int rating, const std::string& comment,
                const std::time_t& created_at, const std::time_t& updated_at);
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    int getUserId() const { return user_id_; }
    void setUserId(int user_id) { user_id_ = user_id; }
    
    int getMovieId() const { return movie_id_; }
    void setMovieId(int movie_id) { movie_id_ = movie_id; }
    
    const std::time_t& getStartTime() const { return start_time_; }
    void setStartTime(const std::time_t& start_time) { start_time_ = start_time; }
    
    int getWatchDuration() const { return watch_duration_; }
    void setWatchDuration(int watch_duration) { watch_duration_ = watch_duration; }
    
    bool isCompleted() const { return is_completed_; }
    void setCompleted(bool completed) { is_completed_ = completed; }
    
    int getRating() const { return rating_; }
    void setRating(int rating) { rating_ = rating; }
    
    const std::string& getComment() const { return comment_; }
    void setComment(const std::string& comment) { comment_ = comment; }
    
    const std::time_t& getCreatedAt() const { return created_at_; }
    void setCreatedAt(const std::time_t& created_at) { created_at_ = created_at; }
    
    const std::time_t& getUpdatedAt() const { return updated_at_; }
    void setUpdatedAt(const std::time_t& updated_at) { updated_at_ = updated_at; }
    
    // 转换为JSON字符串
    std::string toJsonString() const;
    
    // 从map创建
    static WatchRecord fromMap(const std::map<std::string, std::string>& data);
    
    // 转换为map
    std::map<std::string, std::string> toMap() const;
    
private:
    int id_ = 0;
    int user_id_ = 0;
    int movie_id_ = 0;
    std::time_t start_time_ = 0;
    int watch_duration_ = 0;
    bool is_completed_ = false;
    int rating_ = 0;
    std::string comment_;
    std::time_t created_at_ = 0;
    std::time_t updated_at_ = 0;
};

} // namespace model

#endif // WATCH_RECORD_H
