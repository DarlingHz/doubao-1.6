#include "dao/BudgetDAO.h"
#include <sqlite3.h>
#include <iostream>

namespace accounting {

BudgetDAO::BudgetDAO(Database& db) : db_(db) {
}

bool BudgetDAO::upsert(const Budget& budget) {
    const char* sql = R"(
        INSERT OR REPLACE INTO budgets (id, category_id, month, limit) 
        VALUES (
            COALESCE((SELECT id FROM budgets WHERE category_id = ? AND month = ?), NULL), 
            ?, ?, ?
        )
    )";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, budget.getCategoryId());
    sqlite3_bind_text(stmt, 2, budget.getMonth().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, budget.getCategoryId());
    sqlite3_bind_text(stmt, 4, budget.getMonth().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, budget.getLimit());
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Budget> BudgetDAO::getById(int id) {
    const char* sql = "SELECT id, category_id, month, limit FROM budgets WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    Budget budget;
    bool found = false;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        budget = fromRow(stmt);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found ? std::optional<Budget>(budget) : std::nullopt;
}

std::optional<Budget> BudgetDAO::getByMonthAndCategory(const std::string& month, int categoryId) {
    const char* sql = "SELECT id, category_id, month, limit FROM budgets WHERE month = ? AND category_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, categoryId);
    
    Budget budget;
    bool found = false;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        budget = fromRow(stmt);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found ? std::optional<Budget>(budget) : std::nullopt;
}

std::vector<Budget> BudgetDAO::getByMonth(const std::string& month) {
    std::vector<Budget> budgets;
    const char* sql = "SELECT id, category_id, month, limit FROM budgets WHERE month = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return budgets;
    }
    
    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        budgets.push_back(fromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return budgets;
}

bool BudgetDAO::remove(int id) {
    const char* sql = "DELETE FROM budgets WHERE id = ?";
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

bool BudgetDAO::exists(int id) {
    const char* sql = "SELECT 1 FROM budgets WHERE id = ? LIMIT 1";
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

Budget BudgetDAO::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    int categoryId = sqlite3_column_int(stmt, 1);
    const char* month = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    double limit = sqlite3_column_double(stmt, 3);
    
    return Budget(id, categoryId, month ? month : "", limit);
}

} // namespace accounting
