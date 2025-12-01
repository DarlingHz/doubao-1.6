#pragma once
#include <sqlite3.h>
#include <string>
#include <memory>

namespace accounting {

class Database {
public:
    Database(const std::string& dbPath = ":memory:");
    ~Database();
    
    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // 允许移动
    Database(Database&&) noexcept;
    Database& operator=(Database&&) noexcept;
    
    bool isOpen() const;
    sqlite3* getHandle() const;
    
    bool execute(const std::string& sql);
    
    // 初始化数据库表
    bool initialize();
    
private:
    sqlite3* db_;
    bool open_;
};

// 数据库单例
class DatabaseSingleton {
public:
    static DatabaseSingleton& getInstance();
    
    Database& getDatabase();
    bool initialize(const std::string& dbPath = "accounting.db");
    
private:
    DatabaseSingleton();
    ~DatabaseSingleton();
    
    std::unique_ptr<Database> db_;
};

} // namespace accounting
