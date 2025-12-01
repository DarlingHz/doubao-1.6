#ifndef USER_H
#define USER_H

#include <string>
#include <chrono>

namespace model {

class User {
public:
    User() = default;
    
    User(int id, const std::string& email, const std::string& passwordHash,
         const std::string& salt, const std::chrono::system_clock::time_point& createdAt,
         const std::chrono::system_clock::time_point& updatedAt)
        : id_(id), email_(email), passwordHash_(passwordHash), salt_(salt),
          createdAt_(createdAt), updatedAt_(updatedAt) {}
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    const std::string& getEmail() const { return email_; }
    void setEmail(const std::string& email) { email_ = email; }
    
    const std::string& getPasswordHash() const { return passwordHash_; }
    void setPasswordHash(const std::string& passwordHash) { passwordHash_ = passwordHash; }
    
    const std::string& getSalt() const { return salt_; }
    void setSalt(const std::string& salt) { salt_ = salt; }
    
    const std::chrono::system_clock::time_point& getCreatedAt() const { return createdAt_; }
    void setCreatedAt(const std::chrono::system_clock::time_point& createdAt) { createdAt_ = createdAt; }
    
    const std::chrono::system_clock::time_point& getUpdatedAt() const { return updatedAt_; }
    void setUpdatedAt(const std::chrono::system_clock::time_point& updatedAt) { updatedAt_ = updatedAt; }
    
private:
    int id_ = 0;
    std::string email_;
    std::string passwordHash_; // 存储密码哈希值
    std::string salt_;         // 存储盐值
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point updatedAt_;
};

} // namespace model

#endif // USER_H