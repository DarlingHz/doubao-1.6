#include "model/Movie.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace model {

Movie::Movie(int id, const std::string& title, const std::vector<std::string>& genres,
             int duration, Status status, const std::time_t& created_at, const std::time_t& updated_at)
    : id_(id), title_(title), genres_(genres), duration_(duration), status_(status),
      created_at_(created_at), updated_at_(updated_at) {}

std::string Movie::toJsonString() const {
    std::stringstream ss;
    ss << "{ "
       << "\"id\": " << id_ << ", "
       << "\"title\": \"" << title_ << "\", "
       << "\"genres\": [";
    
    // 处理类型数组
    for (size_t i = 0; i < genres_.size(); ++i) {
        ss << "\"" << genres_[i] << "\"";
        if (i != genres_.size() - 1) {
            ss << ", ";
        }
    }
    
    ss << "], "
       << "\"duration\": " << duration_ << ", "
       << "\"status\": " << static_cast<int>(status_) << ", "
       << "\"created_at\": " << created_at_ << ", "
       << "\"updated_at\": " << updated_at_
       << " }";
    
    return ss.str();
}

Movie Movie::fromMap(const std::map<std::string, std::string>& data) {
    Movie movie;
    
    if (data.find("id") != data.end()) {
        movie.id_ = std::stoi(data.at("id"));
    }
    if (data.find("title") != data.end()) {
        movie.title_ = data.at("title");
    }
    if (data.find("genres") != data.end()) {
        movie.genresFromJsonString(data.at("genres"));
    }
    if (data.find("duration") != data.end()) {
        movie.duration_ = std::stoi(data.at("duration"));
    }
    if (data.find("status") != data.end()) {
        movie.status_ = static_cast<Status>(std::stoi(data.at("status")));
    }
    if (data.find("created_at") != data.end()) {
        movie.created_at_ = std::stoll(data.at("created_at"));
    }
    if (data.find("updated_at") != data.end()) {
        movie.updated_at_ = std::stoll(data.at("updated_at"));
    }
    
    return movie;
}

std::map<std::string, std::string> Movie::toMap() const {
    std::map<std::string, std::string> data;
    data["id"] = std::to_string(id_);
    data["title"] = title_;
    data["genres"] = genresToJsonString();
    data["duration"] = std::to_string(duration_);
    data["status"] = std::to_string(static_cast<int>(status_));
    data["created_at"] = std::to_string(created_at_);
    data["updated_at"] = std::to_string(updated_at_);
    return data;
}

std::string Movie::genresToJsonString() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < genres_.size(); ++i) {
        ss << "\"" << genres_[i] << "\"";
        if (i != genres_.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

void Movie::genresFromJsonString(const std::string& json_str) {
    genres_.clear();
    std::string str = json_str;
    
    // 移除首尾的括号
    if (!str.empty() && str.front() == '[' && str.back() == ']') {
        str = str.substr(1, str.size() - 2);
    }
    
    // 分割字符串
    size_t pos = 0;
    std::string delimiter = ", ";
    while ((pos = str.find(delimiter)) != std::string::npos) {
        std::string genre = str.substr(0, pos);
        
        // 移除引号
        if (!genre.empty() && genre.front() == '"' && genre.back() == '"') {
            genre = genre.substr(1, genre.size() - 2);
        }
        
        genres_.push_back(genre);
        str.erase(0, pos + delimiter.length());
    }
    
    // 处理最后一个元素
    if (!str.empty()) {
        // 移除引号
        if (str.front() == '"' && str.back() == '"') {
            str = str.substr(1, str.size() - 2);
        }
        genres_.push_back(str);
    }
}

} // namespace model