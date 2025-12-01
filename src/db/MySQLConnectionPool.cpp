#include "include/db/ConnectionPool.h"
#include <mysql/mysql.h>
#include <iostream>
#include <chrono>
#include <thread>

namespace db {

// MySQL连接实现
class MySQLConnection : public DatabaseConnection {
public:
    MySQLConnection(MYSQL* mysql)
        : mysql_(mysql) {}
    
    ~MySQLConnection() override {
        close();
    }
    
    bool execute(const std::string& sql) override {
        if (!mysql_) return false;
        return mysql_query(mysql_, sql.c_str()) == 0;
    }
    
    std::shared_ptr<void> query(const std::string& sql) override {
        if (!mysql_) return nullptr;
        
        if (mysql_query(mysql_, sql.c_str()) != 0) {
            return nullptr;
        }
        
        MYSQL_RES* result = mysql_store_result(mysql_);
        if (!result) {
            return nullptr;
        }
        
        return std::shared_ptr<MYSQL_RES>(result, mysql_free_result);
    }
    
    int lastInsertId() override {
        if (!mysql_) return 0;
        return static_cast<int>(mysql_insert_id(mysql_));
    }
    
    void close() override {
        if (mysql_) {
            mysql_close(mysql_);
            mysql_ = nullptr;
        }
    }
    
    bool isOpen() const override {
        return mysql_ != nullptr;
    }
    
    MYSQL* getMySQL() const {
        return mysql_;
    }
    
private:
    MYSQL* mysql_ = nullptr;
};

// MySQL连接池实现
class MySQLConnectionPool : public ConnectionPool {
public:
    MySQLConnectionPool() : initialized_(false) {}
    
    ~MySQLConnectionPool() override {
        shutdown();
    }
    
    bool initialize(const DatabaseConfig& config) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            return true;
        }
        
        config_ = config;
        
        // 预创建连接
        for (int i = 0; i < config_.connectionPoolSize; ++i) {
            auto connection = createConnection();
            if (connection) {
                connections_.push(connection);
            }
        }
        
        initialized_ = !connections_.empty();
        return initialized_;
    }
    
    std::shared_ptr<DatabaseConnection> getConnection() override {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 等待可用连接
        while (connections_.empty()) {
            // 如果没有达到最大连接数，创建新连接
            if (activeConnections_ < config_.connectionPoolSize) {
                auto connection = createConnection();
                if (connection) {
                    activeConnections_++;
                    return connection;
                }
            }
            
            // 等待连接释放
            condition_.wait(lock);
        }
        
        auto connection = connections_.front();
        connections_.pop();
        activeConnections_++;
        
        return connection;
    }
    
    void releaseConnection(std::shared_ptr<DatabaseConnection> connection) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (connection && connection->isOpen()) {
            connections_.push(connection);
        }
        
        activeConnections_--;
        condition_.notify_one();
    }
    
    void shutdown() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        while (!connections_.empty()) {
            auto connection = connections_.front();
            connections_.pop();
            connection->close();
        }
        
        initialized_ = false;
        activeConnections_ = 0;
    }
    
private:
    std::shared_ptr<DatabaseConnection> createConnection() {
        MYSQL* mysql = mysql_init(nullptr);
        if (!mysql) {
            return nullptr;
        }
        
        // 设置连接超时
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, "5");
        
        // 连接数据库
        if (!mysql_real_connect(
                mysql,
                config_.host.c_str(),
                config_.username.c_str(),
                config_.password.c_str(),
                config_.database.c_str(),
                config_.port,
                nullptr,
                0)) {
            mysql_close(mysql);
            return nullptr;
        }
        
        // 设置字符集
        mysql_set_character_set(mysql, "utf8mb4");
        
        return std::make_shared<MySQLConnection>(mysql);
    }
    
    DatabaseConfig config_;
    bool initialized_;
    int activeConnections_ = 0;
    std::queue<std::shared_ptr<DatabaseConnection>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

// 获取单例实例
std::shared_ptr<ConnectionPool> ConnectionPool::getInstance() {
    static std::shared_ptr<ConnectionPool> instance = std::make_shared<MySQLConnectionPool>();
    return instance;
}

} // namespace db