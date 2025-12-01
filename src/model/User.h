#ifndef USER_H
#define USER_H

#include <string>
#include <ctime>
#include <map>

namespace model {

class User {
public:
    User() = default;
    User(int id, const std::string& nickname, const std::time_t& created_at, const std::time_t& updated_at);
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getNickname() const { return nickname_; }
    void setNickname(const std::string& nickname) { nickname_ = nickname; }
    
    const std::time_t& getCreatedAt() const { return created_at_; }
    void setCreatedAt(const std::time_t& created_at) { created_at_ = created_at; }
    
    const std::time_t& getUpdatedAt() const { return updated_at_; }
    void setUpdatedAt(const std::time_t& updated_at) { updated_at_ = updated_at; }
    
    // 转换为JSON格式的字符串
    std::string toJsonString() const;
    
    // 从键值对映射创建
    static User fromMap(const std::map<std::string, std::string>& data);
    
    // 转换为键值对映射
    std::map<std::string, std::string> toMap() const;
    
private:
    int id_ = 0;
    std::string nickname_;
    std::time_t created_at_ = 0;
    std::time_t updated_at_ = 0;
};

} // namespace model

#endif // USER_H
