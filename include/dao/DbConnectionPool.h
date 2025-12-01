#pragma once
#include <sqlite3.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>

namespace dao {

class DbConnection {
public:
    explicit DbConnection(sqlite3* db)
        : db_(db) {}
    
    ~DbConnection() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }
    
    sqlite3* get() const {
        return db_;
    }
    
private:
    sqlite3* db_ = nullptr;
};

class DbConnectionPool {
public:
    explicit DbConnectionPool(const std::string& db_path, int pool_size = 5);
    ~DbConnectionPool();
    
    std::shared_ptr<DbConnection> getConnection();
    void returnConnection(std::shared_ptr<DbConnection> connection);
    
private:
    std::string db_path_;
    int pool_size_;
    std::vector<std::shared_ptr<DbConnection>> all_connections_;
    std::queue<std::shared_ptr<DbConnection>> available_connections_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool initialized_ = false;
    
    void initialize();
};

} // namespace dao
