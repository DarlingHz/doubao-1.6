#include "dao/AccountDAO.h"
#include <sqlite3.h>
#include <iostream>

namespace accounting {

AccountDAO::AccountDAO(Database& db) : db_(db) {
}

bool AccountDAO::create(const Account& account) {
    const char* sql = "INSERT INTO accounts (name, type, initial_balance) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, account.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, account.getInitialBalance());
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Account> AccountDAO::getById(int id) {
    const char* sql = "SELECT id, name, type, initial_balance FROM accounts WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    Account account;
    bool found = false;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        account = fromRow(stmt);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found ? std::optional<Account>(account) : std::nullopt;
}

std::vector<Account> AccountDAO::getAll(const std::string& type, int page, int pageSize) {
    std::vector<Account> accounts;
    std::string sql;
    
    if (type.empty()) {
        sql = "SELECT id, name, type, initial_balance FROM accounts LIMIT ? OFFSET ?";
    } else {
        sql = "SELECT id, name, type, initial_balance FROM accounts WHERE type = ? LIMIT ? OFFSET ?";
    }
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return accounts;
    }
    
    int paramIndex = 1;
    if (!type.empty()) {
        sqlite3_bind_text(stmt, paramIndex++, type.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, paramIndex++, pageSize);
    sqlite3_bind_int(stmt, paramIndex++, (page - 1) * pageSize);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        accounts.push_back(fromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return accounts;
}

bool AccountDAO::update(const Account& account) {
    const char* sql = "UPDATE accounts SET name = ?, type = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, account.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, account.getId());
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool AccountDAO::remove(int id) {
    const char* sql = "DELETE FROM accounts WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool AccountDAO::exists(int id) {
    const char* sql = "SELECT 1 FROM accounts WHERE id = ? LIMIT 1";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

int AccountDAO::getCount(const std::string& type) {
    std::string sql;
    if (type.empty()) {
        sql = "SELECT COUNT(*) FROM accounts";
    } else {
        sql = "SELECT COUNT(*) FROM accounts WHERE type = ?";
    }
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return 0;
    }
    
    if (!type.empty()) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

Account AccountDAO::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    double initialBalance = sqlite3_column_double(stmt, 3);
    
    return Account(id, name ? name : "", type ? type : "", initialBalance);
}

} // namespace accounting
