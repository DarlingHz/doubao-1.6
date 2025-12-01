#pragma once
#include "models/Account.h"
#include "dao/AccountDAO.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>

namespace accounting {

class AccountService {
public:
    AccountService(Database& db);
    
    // 创建账户
    std::optional<Account> createAccount(const Account& account);
    
    // 获取账户详情
    std::optional<Account> getAccountById(int id);
    
    // 获取账户列表
    std::vector<Account> getAccounts(const std::string& type = "", int page = 1, int pageSize = 20);
    
    // 更新账户
    bool updateAccount(const Account& account);
    
    // 删除账户
    bool deleteAccount(int id);
    
    // 获取账户总数
    int getAccountCount(const std::string& type = "");
    
private:
    AccountDAO dao_;
    
    // 验证账户数据
    bool validateAccount(const Account& account);
};

} // namespace accounting
