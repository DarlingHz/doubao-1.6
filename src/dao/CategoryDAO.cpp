#include "dao/CategoryDAO.h"
#include <sqlite3.h>
#include <iostream>

namespace accounting {

CategoryDAO::CategoryDAO(Database& db) : db_(db) {
}

bool CategoryDAO::create(const Category& category) {
    const char* sql = "INSERT INTO categories (name, type) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, category.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, category.getType().c_str(), -1, SQLITE_TRANSIENT);
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Category> CategoryDAO::getById(int id) {
    const char* sql = "SELECT id, name, type FROM categories WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return std::nullopt;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    Category category;
    bool found = false;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        category = fromRow(stmt);
        found = true;
    }
    
    sqlite3_finalize(stmt);
    return found ? std::optional<Category>(category) : std::nullopt;
}

std::vector<Category> CategoryDAO::getAll(const std::string& type) {
    std::vector<Category> categories;
    std::string sql;
    
    if (type.empty()) {
        sql = "SELECT id, name, type FROM categories";
    } else {
        sql = "SELECT id, name, type FROM categories WHERE type = ?";
    }
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_.getHandle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return categories;
    }
    
    if (!type.empty()) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        categories.push_back(fromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return categories;
}

bool CategoryDAO::update(const Category& category) {
    const char* sql = "UPDATE categories SET name = ?, type = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_.getHandle()) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, category.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, category.getType().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, category.getId());
    
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool CategoryDAO::remove(int id) {
    const char* sql = "DELETE FROM categories WHERE id = ?";
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

bool CategoryDAO::exists(int id) {
    const char* sql = "SELECT 1 FROM categories WHERE id = ? LIMIT 1";
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

Category CategoryDAO::fromRow(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    
    return Category(id, name ? name : "", type ? type : "");
}

} // namespace accounting
