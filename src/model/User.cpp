#include "model/User.h"
#include <sstream>
#include <iomanip>

namespace model {

User::User(int id, const std::string& nickname, const std::time_t& created_at, const std::time_t& updated_at)
    : id_(id), nickname_(nickname), created_at_(created_at), updated_at_(updated_at) {}

std::string User::toJsonString() const {
    std::stringstream ss;
    ss << "{ " 
       << "\"id\": " << id_ << ", "
       << "\"nickname\": \"" << nickname_ << "\", "
       << "\"created_at\": " << created_at_ << ", "
       << "\"updated_at\": " << updated_at_
       << "}";
    return ss.str();
}

User User::fromMap(const std::map<std::string, std::string>& data) {
    User user;
    
    if (data.find("id") != data.end()) {
        user.id_ = std::stoi(data.at("id"));
    }
    if (data.find("nickname") != data.end()) {
        user.nickname_ = data.at("nickname");
    }
    if (data.find("created_at") != data.end()) {
        user.created_at_ = std::stoll(data.at("created_at"));
    }
    if (data.find("updated_at") != data.end()) {
        user.updated_at_ = std::stoll(data.at("updated_at"));
    }
    
    return user;
}

std::map<std::string, std::string> User::toMap() const {
    std::map<std::string, std::string> data;
    data["id"] = std::to_string(id_);
    data["nickname"] = nickname_;
    data["created_at"] = std::to_string(created_at_);
    data["updated_at"] = std::to_string(updated_at_);
    return data;
}

} // namespace model
