#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "DatabaseConfig.h"

namespace db {

class DatabaseConnection {
public:
    virtual ~DatabaseConnection() = default;
    virtual bool execute(const std::string& sql) = 0;
    virtual std::shared_ptr<void> query(const std::string& sql) = 0;
    virtual int lastInsertId() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

class ConnectionPool {
public:
    virtual ~ConnectionPool() = default;
    
    // 初始化连接池
    virtual bool initialize(const DatabaseConfig& config) = 0;
    
    // 获取连接
    virtual std::shared_ptr<DatabaseConnection> getConnection() = 0;
    
    // 释放连接
    virtual void releaseConnection(std::shared_ptr<DatabaseConnection> connection) = 0;
    
    // 关闭连接池
    virtual void shutdown() = 0;
    
    // 获取单例实例
    static std::shared_ptr<ConnectionPool> getInstance();
};

} // namespace db

#endif // CONNECTION_POOL_H