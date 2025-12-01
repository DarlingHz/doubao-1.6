#include "repository/db_connection_pool.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace api_quota::repository {

// SqliteConnection 实现
SqliteConnection::SqliteConnection(sqlite3* db, const std::string& db_path)
    : db_(db), db_path_(db_path), in_transaction_(false) {
    if (!db) {
        throw DbException("Invalid database connection");
    }
}

SqliteConnection::~SqliteConnection() {
    close();
}

SqliteConnection::SqliteConnection(SqliteConnection&& other) noexcept
    : db_(other.db_), db_path_(std::move(other.db_path_)), in_transaction_(other.in_transaction_) {
    other.db_ = nullptr;
    other.in_transaction_ = false;
}

SqliteConnection& SqliteConnection::operator=(SqliteConnection&& other) noexcept {
    if (this != &other) {
        close();
        db_ = other.db_;
        db_path_ = std::move(other.db_path_);
        in_transaction_ = other.in_transaction_;
        other.db_ = nullptr;
        other.in_transaction_ = false;
    }
    return *this;
}

sqlite3* SqliteConnection::get_db() const {
    return db_;
}

bool SqliteConnection::is_valid() const {
    if (!db_) return false;
    
    // 通过执行简单的PRAGMA语句来验证连接
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, "PRAGMA integrity_check;", nullptr, nullptr, &errmsg);
    if (errmsg) {
        sqlite3_free(errmsg);
    }
    
    return rc == SQLITE_OK;
}

void SqliteConnection::reset() {
    if (in_transaction_) {
        rollback_transaction();
    }
    
    // 重置连接状态（但不关闭）
    char* errmsg = nullptr;
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);
    if (errmsg) {
        sqlite3_free(errmsg);
    }
}

void SqliteConnection::execute(const std::string& sql) {
    if (!db_) {
        throw DbException("Connection closed");
    }
    
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
    
    std::string error_message;
    if (errmsg) {
        error_message = errmsg;
        sqlite3_free(errmsg);
    }
    
    if (rc != SQLITE_OK) {
        throw DbException("Failed to execute SQL: " + error_message, rc);
    }
}

void SqliteConnection::begin_transaction() {
    if (in_transaction_) {
        throw DbException("Transaction already active");
    }
    execute("BEGIN TRANSACTION;");
    in_transaction_ = true;
}

void SqliteConnection::commit_transaction() {
    if (!in_transaction_) {
        throw DbException("No active transaction");
    }
    execute("COMMIT;");
    in_transaction_ = false;
}

void SqliteConnection::rollback_transaction() {
    if (!in_transaction_) {
        return; // 没有活动事务，不需要回滚
    }
    
    // 使用try-catch避免在回滚失败时抛出异常（因为这可能是在析构函数中调用）
    try {
        execute("ROLLBACK;");
    } catch (...) {
        // 静默处理，因为我们已经在尝试恢复
    }
    in_transaction_ = false;
}

void SqliteConnection::close() {
    if (db_) {
        try {
            if (in_transaction_) {
                rollback_transaction();
            }
            sqlite3_close(db_);
        } catch (...) {
            // 静默处理关闭时的错误
        }
        db_ = nullptr;
    }
}

// DbConnectionPool 实现
DbConnectionPool::DbConnectionPool(const std::string& db_path, size_t pool_size)
    : db_path_(db_path), pool_size_(pool_size), used_count_(0), initialized_(false), closed_(false) {
    if (pool_size == 0) {
        throw DbException("Pool size must be greater than 0");
    }
}

DbConnectionPool::~DbConnectionPool() {
    close_all();
}

DbConnectionPool::DbConnectionPool(DbConnectionPool&& other) noexcept
    : db_path_(std::move(other.db_path_)),
      pool_size_(other.pool_size_),
      used_count_(other.used_count_),
      initialized_(other.initialized_),
      closed_(other.closed_) {
    
    // 转移连接队列（需要锁定other的mutex）
    std::lock_guard<std::mutex> lock(other.mutex_);
    available_connections_ = std::move(other.available_connections_);
    
    // 重置other的状态
    other.pool_size_ = 0;
    other.used_count_ = 0;
    other.initialized_ = false;
    other.closed_ = true;
}

DbConnectionPool& DbConnectionPool::operator=(DbConnectionPool&& other) noexcept {
    if (this != &other) {
        close_all();
        
        db_path_ = std::move(other.db_path_);
        pool_size_ = other.pool_size_;
        
        // 转移连接队列（需要锁定other的mutex）
        std::lock_guard<std::mutex> lock(other.mutex_);
        available_connections_ = std::move(other.available_connections_);
        used_count_ = other.used_count_;
        initialized_ = other.initialized_;
        closed_ = other.closed_;
        
        // 重置other的状态
        other.pool_size_ = 0;
        other.used_count_ = 0;
        other.initialized_ = false;
        other.closed_ = true;
    }
    return *this;
}

std::shared_ptr<SqliteConnection> DbConnectionPool::get_connection() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (closed_) {
        throw DbException("Connection pool is closed");
    }
    
    if (!initialized_) {
        initialize();
    }
    
    // 尝试获取可用连接，或者等待
    auto wait_result = condition_.wait_for(lock, std::chrono::seconds(30), [this] {
        return !available_connections_.empty() || used_count_ < pool_size_;
    });
    
    if (!wait_result) {
        throw DbException("Timeout waiting for database connection");
    }
    
    std::shared_ptr<SqliteConnection> connection;
    
    if (!available_connections_.empty()) {
        connection = available_connections_.front();
        available_connections_.pop();
        
        // 验证连接是否仍然有效
        if (!validate_connection(connection)) {
            try {
                connection = create_connection();
            } catch (const std::exception& e) {
                // 如果创建新连接失败，尝试获取下一个可用连接
                if (!available_connections_.empty()) {
                    connection = available_connections_.front();
                    available_connections_.pop();
                } else {
                    throw;
                }
            }
        } else {
            // 重置连接状态
            try {
                connection->reset();
            } catch (const std::exception&) {
                // 如果重置失败，创建新连接
                connection = create_connection();
            }
        }
    } else {
        // 创建新连接
        connection = create_connection();
    }
    
    used_count_++;
    
    // 使用自定义删除器，当shared_ptr被销毁时自动归还连接
    return std::shared_ptr<SqliteConnection>(connection.get(), 
        [this, weak_connection = std::weak_ptr<SqliteConnection>(connection)] 
        (SqliteConnection* conn) {
            if (auto strong_connection = weak_connection.lock()) {
                this->release_connection(strong_connection);
            }
        });
}

void DbConnectionPool::release_connection(std::shared_ptr<SqliteConnection> connection) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (closed_) {
        return; // 连接池已关闭，不归还连接
    }
    
    if (connection && validate_connection(connection)) {
        available_connections_.push(connection);
    }
    
    if (used_count_ > 0) {
        used_count_--;
    }
    
    condition_.notify_one(); // 通知一个等待的线程
}

size_t DbConnectionPool::available_connections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_connections_.size();
}

size_t DbConnectionPool::used_connections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return used_count_;
}

void DbConnectionPool::close_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (closed_) {
        return;
    }
    
    closed_ = true;
    
    // 清空可用连接队列
    std::queue<std::shared_ptr<SqliteConnection>> empty;
    std::swap(available_connections_, empty);
    
    used_count_ = 0;
    initialized_ = false;
    
    // 通知所有等待的线程
    condition_.notify_all();
}

void DbConnectionPool::initialize() {
    if (initialized_ || closed_) {
        return;
    }
    
    // 预创建一些连接
    const size_t initial_connections = std::min(size_t(5), pool_size_);
    for (size_t i = 0; i < initial_connections; i++) {
        try {
            auto connection = create_connection();
            available_connections_.push(connection);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create initial database connection: " << e.what() << std::endl;
            // 继续尝试创建其他连接
        }
    }
    
    initialized_ = true;
}

bool DbConnectionPool::is_initialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

std::shared_ptr<SqliteConnection> DbConnectionPool::create_connection() {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_path_.c_str(), &db);
    
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw DbException("Failed to open database connection: " + error, rc);
    }
    
    // 设置连接参数
    sqlite3_busy_timeout(db, 5000); // 5秒超时
    
    // 开启外键约束
    char* errmsg = nullptr;
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);
    if (errmsg) {
        sqlite3_free(errmsg);
    }
    
    // 设置线程安全模式
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, &errmsg);
    if (errmsg) {
        sqlite3_free(errmsg);
    }
    
    return std::make_shared<SqliteConnection>(db, db_path_);
}

bool DbConnectionPool::validate_connection(std::shared_ptr<SqliteConnection> connection) {
    if (!connection) {
        return false;
    }
    
    try {
        return connection->is_valid();
    } catch (const std::exception&) {
        return false;
    }
}

// DbException 实现
DbException::DbException(const std::string& message)
    : std::runtime_error(message), error_code_(0) {}

DbException::DbException(const std::string& message, int error_code)
    : std::runtime_error(message), error_code_(error_code) {}

int DbException::get_error_code() const {
    return error_code_;
}

// Transaction 实现
Transaction::Transaction(std::shared_ptr<SqliteConnection> connection)
    : connection_(connection), ended_(false), committed_(false) {
    if (!connection_) {
        throw DbException("Invalid database connection");
    }
    connection_->begin_transaction();
}

Transaction::~Transaction() {
    if (!ended_ && connection_) {
        try {
            rollback();
        } catch (...) {
            // 静默处理析构函数中的异常
        }
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : connection_(std::move(other.connection_)),
      ended_(other.ended_),
      committed_(other.committed_) {
    other.ended_ = true; // 防止other的析构函数回滚
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        // 回滚当前事务
        if (!ended_ && connection_) {
            try {
                rollback();
            } catch (...) {
                // 静默处理
            }
        }
        
        connection_ = std::move(other.connection_);
        ended_ = other.ended_;
        committed_ = other.committed_;
        
        other.ended_ = true; // 防止other的析构函数回滚
    }
    return *this;
}

void Transaction::commit() {
    if (ended_ || !connection_) {
        throw DbException("Transaction already ended or invalid connection");
    }
    
    connection_->commit_transaction();
    ended_ = true;
    committed_ = true;
}

void Transaction::rollback() {
    if (ended_ || !connection_) {
        return; // 事务已结束或连接无效，不需要回滚
    }
    
    connection_->rollback_transaction();
    ended_ = true;
    committed_ = false;
}

bool Transaction::is_ended() const {
    return ended_;
}

} // namespace api_quota::repository