#include <dao/DbConnectionPool.h>
#include <util/Logger.h>
#include <stdexcept>

namespace dao {

DbConnectionPool::DbConnectionPool(const std::string& db_path, int pool_size)
    : db_path_(db_path), pool_size_(pool_size) {
    if (pool_size <= 0) {
        throw std::invalid_argument("连接池大小必须大于0");
    }
    initialize();
}

DbConnectionPool::~DbConnectionPool() {
    // 释放所有连接
    std::lock_guard<std::mutex> lock(mutex_);
    all_connections_.clear();
    while (!available_connections_.empty()) {
        available_connections_.pop();
    }
}

void DbConnectionPool::initialize() {
    try {
        for (int i = 0; i < pool_size_; ++i) {
            sqlite3* db = nullptr;
            int result = sqlite3_open(db_path_.c_str(), &db);
            if (result != SQLITE_OK) {
                LOG_ERROR("打开数据库连接失败: " + std::string(sqlite3_errmsg(db)));
                sqlite3_close(db);
                throw std::runtime_error("初始化数据库连接池失败");
            }
            
            // 启用外键约束
            sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
            
            auto connection = std::make_shared<DbConnection>(db);
            all_connections_.push_back(connection);
            available_connections_.push(connection);
        }
        
        initialized_ = true;
        LOG_INFO("数据库连接池初始化成功，大小: " + std::to_string(pool_size_));
    } catch (const std::exception& e) {
        LOG_CRITICAL("初始化数据库连接池异常: " + std::string(e.what()));
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 等待直到有可用连接
    cv_.wait(lock, [this]() { return !available_connections_.empty(); });
    
    auto connection = available_connections_.front();
    available_connections_.pop();
    
    return connection;
}

void DbConnectionPool::returnConnection(std::shared_ptr<DbConnection> connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查连接是否属于这个池
    bool belongs_to_pool = false;
    for (const auto& conn : all_connections_) {
        if (conn == connection) {
            belongs_to_pool = true;
            break;
        }
    }
    
    if (belongs_to_pool) {
        available_connections_.push(connection);
        cv_.notify_one();
    } else {
        LOG_WARNING("尝试归还不属于此连接池的连接");
    }
}

} // namespace dao
