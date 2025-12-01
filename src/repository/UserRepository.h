#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "repository/Database.h"
#include "model/User.h"
#include <vector>
#include <optional>

namespace repository {

class UserRepository {
public:
    explicit UserRepository(DatabasePtr db);
    
    // 创建用户
    std::optional<model::User> create(const model::User& user);
    
    // 根据ID查询用户
    std::optional<model::User> findById(int id);
    
    // 查询所有用户
    std::vector<model::User> findAll();
    
    // 根据昵称查询用户
    std::optional<model::User> findByNickname(const std::string& nickname);
    
    // 更新用户信息
    bool update(const model::User& user);
    
private:
    DatabasePtr db_;
    
    // 从SQL结果构建用户对象
    static model::User buildUserFromRow(char** row);
};

} // namespace repository

#endif // USER_REPOSITORY_H
