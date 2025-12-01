#include "model/WatchRecord.h"
#include <sstream>
#include <iomanip>

namespace model {

WatchRecord::WatchRecord(int id, int user_id, int movie_id, const std::time_t& start_time,
                         int watch_duration, bool is_completed, int rating, const std::string& comment,
                         const std::time_t& created_at, const std::time_t& updated_at)
    : id_(id), user_id_(user_id), movie_id_(movie_id), start_time_(start_time),
      watch_duration_(watch_duration), is_completed_(is_completed), rating_(rating),
      comment_(comment), created_at_(created_at), updated_at_(updated_at) {}

std::string WatchRecord::toJsonString() const {
    std::stringstream ss;
    ss << "{ "
       << "\"id\": " << id_ << ", "
       << "\"user_id\": " << user_id_ << ", "
       << "\"movie_id\": " << movie_id_ << ", "
       << "\"start_time\": " << start_time_ << ", "
       << "\"watch_duration\": " << watch_duration_ << ", "
       << "\"is_completed\": " << (is_completed_ ? "true" : "false") << ", ";
    
    if (rating_ > 0) {
        ss << "\"rating\": " << rating_ << ", ";
    }
    
    if (!comment_.empty()) {
        ss << "\"comment\": \"" << comment_ << "\", ";
    }
    
    ss << "\"created_at\": " << created_at_ << ", "
       << "\"updated_at\": " << updated_at_
       << " }";
    
    return ss.str();
}

WatchRecord WatchRecord::fromMap(const std::map<std::string, std::string>& data) {
    WatchRecord record;
    
    if (data.find("id") != data.end()) {
        record.id_ = std::stoi(data.at("id"));
    }
    if (data.find("user_id") != data.end()) {
        record.user_id_ = std::stoi(data.at("user_id"));
    }
    if (data.find("movie_id") != data.end()) {
        record.movie_id_ = std::stoi(data.at("movie_id"));
    }
    if (data.find("start_time") != data.end()) {
        record.start_time_ = std::stoll(data.at("start_time"));
    }
    if (data.find("watch_duration") != data.end()) {
        record.watch_duration_ = std::stoi(data.at("watch_duration"));
    }
    if (data.find("is_completed") != data.end()) {
        record.is_completed_ = (data.at("is_completed") == "true" || data.at("is_completed") == "1");
    }
    if (data.find("rating") != data.end()) {
        record.rating_ = std::stoi(data.at("rating"));
    }
    if (data.find("comment") != data.end()) {
        record.comment_ = data.at("comment");
    }
    if (data.find("created_at") != data.end()) {
        record.created_at_ = std::stoll(data.at("created_at"));
    }
    if (data.find("updated_at") != data.end()) {
        record.updated_at_ = std::stoll(data.at("updated_at"));
    }
    
    return record;
}

std::map<std::string, std::string> WatchRecord::toMap() const {
    std::map<std::string, std::string> data;
    data["id"] = std::to_string(id_);
    data["user_id"] = std::to_string(user_id_);
    data["movie_id"] = std::to_string(movie_id_);
    data["start_time"] = std::to_string(start_time_);
    data["watch_duration"] = std::to_string(watch_duration_);
    data["is_completed"] = is_completed_ ? "true" : "false";
    data["rating"] = std::to_string(rating_);
    data["comment"] = comment_;
    data["created_at"] = std::to_string(created_at_);
    data["updated_at"] = std::to_string(updated_at_);
    return data;
}

} // namespace model
