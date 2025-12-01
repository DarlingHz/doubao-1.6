#include "dao/Database.h"
#include <stdexcept>
#include <iostream>

namespace accounting {

Database::Database(const std::string& dbPath) : db_(nullptr), open_(false) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
    } else {
        open_ = true;
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        open_ = false;
    }
}

Database::Database(Database&& other) noexcept : db_(other.db_), open_(other.open_) {
    other.db_ = nullptr;
    other.open_ = false;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        open_ = other.open_;
        other.db_ = nullptr;
        other.open_ = false;
    }
    return *this;
}

bool Database::isOpen() const {
    return open_;
}

sqlite3* Database::getHandle() const {
    return db_;
}

bool Database::execute(const std::string& sql) {
    if (!open_) {
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::initialize() {
    if (!open_) {
        return false;
    }
    
    // 创建账户表
    std::string createAccountsTable = R"(
        CREATE TABLE IF NOT EXISTS accounts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            initial_balance REAL DEFAULT 0.0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    // 创建分类表
    std::string createCategoriesTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    // 创建交易记录表
    std::string createTransactionsTable = R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account_id INTEGER NOT NULL,
            category_id INTEGER NOT NULL,
            type TEXT NOT NULL,
            amount REAL NOT NULL,
            time TEXT NOT NULL,
            note TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE CASCADE
        );
    )";
    
    // 创建预算表
    std::string createBudgetsTable = R"(
        CREATE TABLE IF NOT EXISTS budgets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER NOT NULL,
            month TEXT NOT NULL,
            limit REAL NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE CASCADE,
            UNIQUE(category_id, month)
        );
    )";
    
    // 创建索引
    std::string createIndexes = R"(
        CREATE INDEX IF NOT EXISTS idx_transactions_time ON transactions(time);
        CREATE INDEX IF NOT EXISTS idx_transactions_account ON transactions(account_id);
        CREATE INDEX IF NOT EXISTS idx_transactions_category ON transactions(category_id);
        CREATE INDEX IF NOT EXISTS idx_transactions_type ON transactions(type);
        CREATE INDEX IF NOT EXISTS idx_budgets_month ON budgets(month);
        CREATE INDEX IF NOT EXISTS idx_budgets_category ON budgets(category_id);
    )";
    
    return execute(createAccountsTable) &&
           execute(createCategoriesTable) &&
           execute(createTransactionsTable) &&
           execute(createBudgetsTable) &&
           execute(createIndexes);
}

// DatabaseSingleton 实现
DatabaseSingleton& DatabaseSingleton::getInstance() {
    static DatabaseSingleton instance;
    return instance;
}

DatabaseSingleton::DatabaseSingleton() : db_(nullptr) {
}

DatabaseSingleton::~DatabaseSingleton() {
    // db_ 会在 unique_ptr 析构时自动关闭
}

Database& DatabaseSingleton::getDatabase() {
    if (!db_) {
        throw std::runtime_error("Database not initialized");
    }
    return *db_;
}

bool DatabaseSingleton::initialize(const std::string& dbPath) {
    db_ = std::make_unique<Database>(dbPath);
    return db_->isOpen() && db_->initialize();
}

} // namespace accounting
