#include "dao/TransactionDAO.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

namespace accounting {

TransactionDAO::TransactionDAO(Database& db) : db_(db) {
}

bool TransactionDAO::create(const Transaction& transaction) {
    const char* sql = "INSERT INTO transactions (account_id, category_id, type, amount, time, note) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, transaction.getAccountId());
    sqlite3_bind_int(stmt, 2, transaction.getCategoryId());
    sqlite3_bind_text(stmt, 3, transaction.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, transaction.getAmount());
    sqlite3_bind_text(stmt, 5, transaction.timeToString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, transaction.getNote().c_str(), -1, SQLITE_TRANSIENT);
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Transaction> TransactionDAO::getById(int id) {
    const char* sql = "SELECT id, account_id, category_id, type, amount, time, note FROM transactions WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    Transaction transaction;
    bool found = false;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        transaction = fromRow(stmt);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found ? std::optional<Transaction>(transaction) : std::nullopt;
}

std::vector<Transaction> TransactionDAO::query(const TransactionFilter& filter,
                                             const std::string& sortBy,
                                             bool sortDesc,
                                             int page, int pageSize) {
    std::vector<Transaction> transactions;
    std::string sql = buildQuerySQL(filter, sortBy, sortDesc, false);
    
    // 添加分页
    sql += " LIMIT ? OFFSET ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return transactions;
    }
    
    // 绑定过滤参数
    bindQueryParams(stmt, filter);
    
    // 绑定分页参数
    int paramIndex = 1;
    while (sqlite3_bind_parameter_count(stmt) > paramIndex) {
        ++paramIndex;
    }
    sqlite3_bind_int(stmt, paramIndex++, pageSize);
    sqlite3_bind_int(stmt, paramIndex++, (page - 1) * pageSize);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        transactions.push_back(fromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return transactions;
}

bool TransactionDAO::update(const Transaction& transaction) {
    const char* sql = "UPDATE transactions SET account_id = ?, category_id = ?, type = ?, amount = ?, time = ?, note = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, transaction.getAccountId());
    sqlite3_bind_int(stmt, 2, transaction.getCategoryId());
    sqlite3_bind_text(stmt, 3, transaction.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, transaction.getAmount());
    sqlite3_bind_text(stmt, 5, transaction.timeToString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, transaction.getNote().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, transaction.getId());
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool TransactionDAO::remove(int id) {
    const char* sql = "DELETE FROM transactions WHERE id = ?";
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

bool TransactionDAO::exists(int id) {
    const char* sql = "SELECT 1 FROM transactions WHERE id = ? LIMIT 1";
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

int TransactionDAO::getCount(const TransactionFilter& filter) {
    std::string sql = buildQuerySQL(filter, "", false, true);
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return 0;
    }
    
    bindQueryParams(stmt, filter);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

std::pair<double, double> TransactionDAO::getMonthlySummary(const std::string& month) {
    const char* sql = R"(
        SELECT 
            SUM(CASE WHEN type = 'income' THEN amount ELSE 0 END) as income,
            SUM(CASE WHEN type = 'expense' THEN amount ELSE 0 END) as expense
        FROM transactions 
        WHERE time LIKE ?
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return {0.0, 0.0};
    }
    
    std::string monthPattern = month + "%";
    sqlite3_bind_text(stmt, 1, monthPattern.c_str(), -1, SQLITE_TRANSIENT);
    
    double income = 0.0, expense = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        income = sqlite3_column_double(stmt, 0);
        expense = sqlite3_column_double(stmt, 1);
    }
    
    sqlite3_finalize(stmt);
    return {income, expense};
}

std::vector<std::pair<int, double>> TransactionDAO::getCategoryExpenses(const std::string& month) {
    const char* sql = R"(
        SELECT category_id, SUM(amount) as total 
        FROM transactions 
        WHERE type = 'expense' AND time LIKE ? 
        GROUP BY category_id
    )";
    
    std::vector<std::pair<int, double>> expenses;
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return expenses;
    }
    
    std::string monthPattern = month + "%";
    sqlite3_bind_text(stmt, 1, monthPattern.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int categoryId = sqlite3_column_int(stmt, 0);
        double total = sqlite3_column_double(stmt, 1);
        expenses.emplace_back(categoryId, total);
    }
    
    sqlite3_finalize(stmt);
    return expenses;
}

Transaction TransactionDAO::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int accountId = sqlite3_column_int(stmt, 1);
    int categoryId = sqlite3_column_int(stmt, 2);
    const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    double amount = sqlite3_column_double(stmt, 4);
    const char* timeStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    const char* note = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    
    auto time = Transaction::stringToTime(timeStr ? timeStr : "");
    return Transaction(id, accountId, categoryId, type ? type : "", amount, time, note ? note : "");
}

std::string TransactionDAO::buildQuerySQL(const TransactionFilter& filter,
                                         const std::string& sortBy,
                                         bool sortDesc,
                                         bool forCount) {
    std::stringstream ss;
    
    if (forCount) {
        ss << "SELECT COUNT(*) FROM transactions WHERE 1=1";
    } else {
        ss << "SELECT id, account_id, category_id, type, amount, time, note FROM transactions WHERE 1=1";
    }
    
    // 添加过滤条件
    if (filter.accountId > 0) {
        ss << " AND account_id = ?";
    }
    if (filter.categoryId > 0) {
        ss << " AND category_id = ?";
    }
    if (!filter.type.empty()) {
        ss << " AND type = ?";
    }
    if (filter.amountMin > 0) {
        ss << " AND amount >= ?";
    }
    if (filter.amountMax > 0) {
        ss << " AND amount <= ?";
    }
    
    // 时间范围过滤
    ss << " AND time >= ? AND time <= ?";
    
    // 添加排序
    if (!forCount && !sortBy.empty()) {
        ss << " ORDER BY " << (sortBy == "amount" ? "amount" : "time") << " "
           << (sortDesc ? "DESC" : "ASC");
    }
    
    return ss.str();
}

void TransactionDAO::bindQueryParams(sqlite3_stmt* stmt, const TransactionFilter& filter) {
    int paramIndex = 1;
    
    if (filter.accountId > 0) {
        sqlite3_bind_int(stmt, paramIndex++, filter.accountId);
    }
    if (filter.categoryId > 0) {
        sqlite3_bind_int(stmt, paramIndex++, filter.categoryId);
    }
    if (!filter.type.empty()) {
        sqlite3_bind_text(stmt, paramIndex++, filter.type.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (filter.amountMin > 0) {
        sqlite3_bind_double(stmt, paramIndex++, filter.amountMin);
    }
    if (filter.amountMax > 0) {
        sqlite3_bind_double(stmt, paramIndex++, filter.amountMax);
    }
    
    // 绑定时间范围
    Transaction fromTrans, toTrans;
    fromTrans.setTime(filter.fromTime);
    toTrans.setTime(filter.toTime);
    
    sqlite3_bind_text(stmt, paramIndex++, fromTrans.timeToString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, paramIndex++, toTrans.timeToString().c_str(), -1, SQLITE_TRANSIENT);
}

} // namespace accounting
