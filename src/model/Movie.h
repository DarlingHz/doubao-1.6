#ifndef MOVIE_H
#define MOVIE_H

#include <string>
#include <vector>
#include <ctime>
#include <map>

namespace model {

class Movie {
public:
    enum Status {
        DELETED = 0,
        ACTIVE = 1
    };
    
    Movie() = default;
    Movie(int id, const std::string& title, const std::vector<std::string>& genres,
          int duration, Status status, const std::time_t& created_at, const std::time_t& updated_at);
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getTitle() const { return title_; }
    void setTitle(const std::string& title) { title_ = title; }
    
    const std::vector<std::string>& getGenres() const { return genres_; }
    void setGenres(const std::vector<std::string>& genres) { genres_ = genres; }
    
    int getDuration() const { return duration_; }
    void setDuration(int duration) { duration_ = duration; }
    
    Status getStatus() const { return status_; }
    void setStatus(Status status) { status_ = status; }
    
    const std::time_t& getCreatedAt() const { return created_at_; }
    void setCreatedAt(const std::time_t& created_at) { created_at_ = created_at; }
    
    const std::time_t& getUpdatedAt() const { return updated_at_; }
    void setUpdatedAt(const std::time_t& updated_at) { updated_at_ = updated_at; }
    
    // 转换为JSON格式的字符串
    std::string toJsonString() const;
    
    // 从键值对映射创建
    static Movie fromMap(const std::map<std::string, std::string>& data);
    
    // 转换为键值对映射
    std::map<std::string, std::string> toMap() const;
    
    // 将类型数组转换为JSON字符串
    std::string genresToJsonString() const;
    
    // 从JSON字符串解析类型数组
    void genresFromJsonString(const std::string& json_str);
    
private:
    int id_ = 0;
    std::string title_;
    std::vector<std::string> genres_;
    int duration_ = 0;
    Status status_ = ACTIVE;
    std::time_t created_at_ = 0;
    std::time_t updated_at_ = 0;
};

} // namespace model

#endif // MOVIE_H
