#include "database.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

// Database 类实现
Database::Database() : db_(nullptr) {
}

Database::~Database() {
    disconnect();
}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::connect(const std::string& dbPath) {
    if (db_) {
        return true; // 已连接
    }
    
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    // 启用外键约束
    execute("PRAGMA foreign_keys = ON;");
    
    return true;
}

void Database::disconnect() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    if (!db_) {
        errorMessage_ = "Database not connected";
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        errorMessage_ = errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

std::vector<std::map<std::string, std::string>> Database::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> results;
    
    if (!db_) {
        errorMessage_ = "Database not connected";
        return results;
    }
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(db_);
        return results;
    }
    
    // 获取列数
    int columnCount = sqlite3_column_count(stmt);
    std::vector<std::string> columnNames;
    
    // 获取列名
    for (int i = 0; i < columnCount; ++i) {
        columnNames.push_back(sqlite3_column_name(stmt, i));
    }
    
    // 获取结果
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::map<std::string, std::string> row;
        
        for (int i = 0; i < columnCount; ++i) {
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row[columnNames[i]] = (value ? value : "");
        }
        
        results.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

bool Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

bool Database::commitTransaction() {
    return execute("COMMIT TRANSACTION;");
}

bool Database::rollbackTransaction() {
    return execute("ROLLBACK TRANSACTION;");
}

int64_t Database::getLastInsertId() {
    if (!db_) {
        return -1;
    }
    return sqlite3_last_insert_rowid(db_);
}

std::string Database::getErrorMessage() const {
    return errorMessage_;
}

sqlite3* Database::getConnection() {
    return db_;
}

// PreparedStatement 类实现
PreparedStatement::PreparedStatement(sqlite3* db, const std::string& sql) : stmt_(nullptr) {
    if (!db) {
        errorMessage_ = "Database connection is null";
        return;
    }
    
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(db);
    }
}

PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

bool PreparedStatement::bind(int index, const std::string& value) {
    if (!stmt_) return false;
    int rc = sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(sqlite3_db_handle(stmt_));
        return false;
    }
    return true;
}

bool PreparedStatement::bind(int index, int value) {
    if (!stmt_) return false;
    int rc = sqlite3_bind_int(stmt_, index, value);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(sqlite3_db_handle(stmt_));
        return false;
    }
    return true;
}

bool PreparedStatement::bind(int index, int64_t value) {
    if (!stmt_) return false;
    int rc = sqlite3_bind_int64(stmt_, index, value);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(sqlite3_db_handle(stmt_));
        return false;
    }
    return true;
}

bool PreparedStatement::bind(int index, double value) {
    if (!stmt_) return false;
    int rc = sqlite3_bind_double(stmt_, index, value);
    if (rc != SQLITE_OK) {
        errorMessage_ = sqlite3_errmsg(sqlite3_db_handle(stmt_));
        return false;
    }
    return true;
}

bool PreparedStatement::execute() {
    if (!stmt_) return false;
    int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE) {
        errorMessage_ = sqlite3_errmsg(sqlite3_db_handle(stmt_));
        return false;
    }
    return true;
}

bool PreparedStatement::next() {
    if (!stmt_) return false;
    int rc = sqlite3_step(stmt_);
    return rc == SQLITE_ROW;
}

std::string PreparedStatement::getString(int column) {
    if (!stmt_) return "";
    const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
    return (value ? value : "");
}

int PreparedStatement::getInt(int column) {
    if (!stmt_) return 0;
    return sqlite3_column_int(stmt_, column);
}

int64_t PreparedStatement::getInt64(int column) {
    if (!stmt_) return 0;
    return sqlite3_column_int64(stmt_, column);
}

double PreparedStatement::getDouble(int column) {
    if (!stmt_) return 0.0;
    return sqlite3_column_double(stmt_, column);
}

std::string PreparedStatement::getErrorMessage() const {
    return errorMessage_;
}
