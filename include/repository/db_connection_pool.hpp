#ifndef DB_CONNECTION_POOL_HPP
#define DB_CONNECTION_POOL_HPP

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <sqlite3.h>
#include <stdexcept>

namespace api_quota::repository {

// SQLite连接包装类
class SqliteConnection {
public:
    SqliteConnection(sqlite3* db, const std::string& db_path);
    ~SqliteConnection();
    
    // 禁止拷贝
    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;
    
    // 允许移动
    SqliteConnection(SqliteConnection&& other) noexcept;
    SqliteConnection& operator=(SqliteConnection&& other) noexcept;
    
    // 获取原生sqlite3指针
    sqlite3* get_db() const;
    
    // 检查连接是否有效
    bool is_valid() const;
    
    // 重置连接（用于连接池）
    void reset();
    
    // 执行简单SQL语句
    void execute(const std::string& sql);
    
    // 开始事务
    void begin_transaction();
    
    // 提交事务
    void commit_transaction();
    
    // 回滚事务
    void rollback_transaction();
    
private:
    sqlite3* db_;
    std::string db_path_;
    bool in_transaction_;
    
    // 关闭数据库连接
    void close();
};

// 数据库连接池类
class DbConnectionPool {
public:
    DbConnectionPool(const std::string& db_path, size_t pool_size = 10);
    ~DbConnectionPool();
    
    // 禁止拷贝
    DbConnectionPool(const DbConnectionPool&) = delete;
    DbConnectionPool& operator=(const DbConnectionPool&) = delete;
    
    // 允许移动
    DbConnectionPool(DbConnectionPool&& other) noexcept;
    DbConnectionPool& operator=(DbConnectionPool&& other) noexcept;
    
    // 从池中获取连接
    std::shared_ptr<SqliteConnection> get_connection();
    
    // 回收连接到池中
    void release_connection(std::shared_ptr<SqliteConnection> connection);
    
    // 获取当前可用连接数
    size_t available_connections() const;
    
    // 获取当前已使用连接数
    size_t used_connections() const;
    
    // 关闭所有连接
    void close_all();
    
    // 初始化连接池
    void initialize();
    
    // 检查连接池是否已初始化
    bool is_initialized() const;
    
private:
    std::string db_path_;
    size_t pool_size_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::shared_ptr<SqliteConnection>> available_connections_;
    size_t used_count_;
    bool initialized_;
    bool closed_;
    
    // 创建新连接
    std::shared_ptr<SqliteConnection> create_connection();
    
    // 验证连接
    bool validate_connection(std::shared_ptr<SqliteConnection> connection);
};

// 数据库连接异常类
class DbException : public std::runtime_error {
public:
    DbException(const std::string& message);
    DbException(const std::string& message, int error_code);
    
    int get_error_code() const;
    
private:
    int error_code_;
};

// 事务辅助类（RAII模式）
class Transaction {
public:
    Transaction(std::shared_ptr<SqliteConnection> connection);
    ~Transaction();
    
    // 禁止拷贝
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    
    // 允许移动
    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;
    
    // 提交事务
    void commit();
    
    // 回滚事务
    void rollback();
    
    // 检查事务是否已结束
    bool is_ended() const;
    
private:
    std::shared_ptr<SqliteConnection> connection_;
    bool ended_;
    bool committed_;
};

} // namespace api_quota::repository

#endif // DB_CONNECTION_POOL_HPP