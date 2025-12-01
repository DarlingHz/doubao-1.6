#pragma once
#include "models/Account.h"
#include "dao/Database.h"
#include <vector>
#include <optional>

namespace accounting {

class AccountDAO {
public:
    AccountDAO(Database& db);
    
    // 创建账户
    bool create(const Account& account);
    
    // 根据ID获取账户
    std::optional<Account> getById(int id);
    
    // 获取所有账户，支持类型过滤和分页
    std::vector<Account> getAll(const std::string& type = "", int page = 1, int pageSize = 20);
    
    // 更新账户
    bool update(const Account& account);
    
    // 删除账户
    bool remove(int id);
    
    // 检查账户是否存在
    bool exists(int id);
    
    // 获取账户总数
    int getCount(const std::string& type = "");
    
private:
    Database& db_;
    
    // 从数据库行构造Account对象
    Account fromRow(sqlite3_stmt* stmt);
};

} // namespace accounting
