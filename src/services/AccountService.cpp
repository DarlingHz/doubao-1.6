#include "services/AccountService.h"
#include <stdexcept>
#include <iostream>

namespace accounting {

AccountService::AccountService(Database& db) : dao_(db) {
}

std::optional<Account> AccountService::createAccount(const Account& account) {
    if (!validateAccount(account)) {
        std::cerr << "Invalid account data" << std::endl;
        return std::nullopt;
    }
    
    if (!dao_.create(account)) {
        std::cerr << "Failed to create account" << std::endl;
        return std::nullopt;
    }
    
    // 这里简化处理，实际应该通过其他方式获取刚插入的记录
    // 或者在DAO层提供一个方法返回插入后的ID
    std::vector<Account> accounts = dao_.getAll(account.getType(), 1, 1);
    if (!accounts.empty()) {
        return accounts.back(); // 返回最后一个，假设是刚插入的
    }
    
    return std::nullopt;
}

std::optional<Account> AccountService::getAccountById(int id) {
    if (id <= 0) {
        return std::nullopt;
    }
    
    return dao_.getById(id);
}

std::vector<Account> AccountService::getAccounts(const std::string& type, int page, int pageSize) {
    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 20;
    
    return dao_.getAll(type, page, pageSize);
}

bool AccountService::updateAccount(const Account& account) {
    if (account.getId() <= 0) {
        return false;
    }
    
    if (!validateAccount(account)) {
        return false;
    }
    
    if (!dao_.exists(account.getId())) {
        return false;
    }
    
    return dao_.update(account);
}

bool AccountService::deleteAccount(int id) {
    if (id <= 0) {
        return false;
    }
    
    if (!dao_.exists(id)) {
        return false;
    }
    
    return dao_.remove(id);
}

int AccountService::getAccountCount(const std::string& type) {
    return dao_.getCount(type);
}

bool AccountService::validateAccount(const Account& account) {
    // 验证账户名称
    if (account.getName().empty()) {
        return false;
    }
    
    // 验证账户类型
    const std::string& type = account.getType();
    if (type != "cash" && type != "bank" && type != "wallet") {
        return false;
    }
    
    // 验证初始余额
    if (account.getInitialBalance() < 0) {
        return false;
    }
    
    return true;
}

} // namespace accounting
