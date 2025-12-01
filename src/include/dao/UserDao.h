#ifndef USER_DAO_H
#define USER_DAO_H

#include <memory>
#include <string>
#include "model/User.h"

namespace dao {

class UserDao {
public:
    virtual ~UserDao() = default;
    
    // 根据ID查找用户
    virtual std::shared_ptr<model::User> findById(int id) = 0;
    
    // 根据邮箱查找用户
    virtual std::shared_ptr<model::User> findByEmail(const std::string& email) = 0;
    
    // 创建新用户
    virtual int create(const model::User& user) = 0;
    
    // 更新用户信息
    virtual bool update(const model::User& user) = 0;
    
    // 删除用户
    virtual bool deleteById(int id) = 0;
};

} // namespace dao

#endif // USER_DAO_H