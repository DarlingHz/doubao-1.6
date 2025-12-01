#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include "repository/UserRepository.h"
#include "model/User.h"
#include <optional>
#include <string>

namespace service {

class UserService {
public:
    explicit UserService(std::shared_ptr<repository::UserRepository> repository);
    
    // 创建用户
    std::optional<model::User> createUser(const std::string& nickname);
    
    // 根据ID查询用户
    std::optional<model::User> getUserById(int id);
    
    // 验证用户信息
    bool validateUser(const model::User& user);
    
private:
    std::shared_ptr<repository::UserRepository> repository_;
    
    // 验证昵称
    bool validateNickname(const std::string& nickname);
};

} // namespace service

#endif // USER_SERVICE_H
