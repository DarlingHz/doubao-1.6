#include "utils/ConnectionPool.h"
#include "utils/Logger.h"
#include <chrono>
#include <thread>
#include <memory>

namespace utils {

// 模拟数据库连接类
Connection::Connection(const std::string& host, int port, const std::string& dbname,
                     const std::string& user, const std::string& password) {
    connected = true; // 模拟连接始终成功
    connectionHandle = this; // 使用this指针作为模拟的连接句柄
    LOG_DEBUG("创建模拟数据库连接");
}

Connection::~Connection() {
    close();
}

bool Connection::isConnected() const {
    return connected;
}

void* Connection::getNativeHandle() {
    return connectionHandle;
}

void Connection::close() {
    if (connected) {
        connected = false;
        connectionHandle = nullptr;
        LOG_DEBUG("关闭模拟数据库连接");
    }
}

ConnectionPool::ConnectionPool(size_t minConnections, size_t maxConnections,
                             const std::string& host, int port, const std::string& dbname,
                             const std::string& user, const std::string& password)
    : minConnections(minConnections), maxConnections(maxConnections),
      host(host), port(port), dbname(dbname), user(user), password(password),
      activeConnectionCount(0), isShutdown(false) {
    
    initializeConnections();
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

void ConnectionPool::initializeConnections() {
    LOG_INFO("初始化连接池，最小连接数: " + std::to_string(minConnections));
    
    for (size_t i = 0; i < minConnections; ++i) {
        auto conn = createConnection();
        if (conn && conn->isConnected()) {
            idleConnections.push(conn);
        }
    }
}

Connection::Ptr ConnectionPool::createConnection() {
    try {
        auto conn = std::make_shared<Connection>(host, port, dbname, user, password);
        return conn;
    } catch (const std::exception& e) {
        LOG_ERROR("创建模拟数据库连接失败: " + std::string(e.what()));
        return nullptr;
    }
}

Connection::Ptr ConnectionPool::getConnection(int timeoutMs) {
    std::unique_lock<std::mutex> lock(poolMutex);
    
    // 检查是否已关闭
    if (isShutdown) {
        LOG_ERROR("连接池已关闭");
        return nullptr;
    }
    
    // 等待可用连接或超时
    auto predicate = [this]() {
        return !idleConnections.empty() || (activeConnectionCount < maxConnections);
    };
    
    bool waitResult = true;
    if (timeoutMs > 0) {
        waitResult = connectionAvailable.wait_for(lock, 
                                               std::chrono::milliseconds(timeoutMs),
                                               predicate);
    } else {
        connectionAvailable.wait(lock, predicate);
    }
    
    if (!waitResult) {
        LOG_WARNING("获取数据库连接超时");
        return nullptr;
    }
    
    Connection::Ptr connection;
    
    // 优先使用空闲连接
    if (!idleConnections.empty()) {
        connection = idleConnections.front();
        idleConnections.pop();
        
        // 检查连接是否有效
        if (!connection->isConnected()) {
            LOG_DEBUG("发现无效连接，重新创建");
            connection = createConnection();
        }
    } else if (activeConnectionCount < maxConnections) {
        // 创建新连接
        connection = createConnection();
    }
    
    if (connection && connection->isConnected()) {
        activeConnectionCount++;
        LOG_DEBUG("获取模拟数据库连接成功，活跃连接数: " + std::to_string(activeConnectionCount));
    } else {
        LOG_ERROR("无法获取有效的模拟数据库连接");
    }
    
    return connection;
}

void ConnectionPool::returnConnection(Connection::Ptr connection) {
    if (!connection) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (isShutdown) {
        // 如果连接池已关闭，直接关闭连接
        connection->close();
    } else {
        // 检查连接是否有效，如果有效则放回池中
        if (connection->isConnected()) {
            idleConnections.push(connection);
            connectionAvailable.notify_one();
        }
    }
    
    activeConnectionCount--;
    LOG_DEBUG("归还模拟数据库连接，活跃连接数: " + std::to_string(activeConnectionCount));
}

size_t ConnectionPool::getActiveConnections() const {
    return activeConnectionCount;
}

size_t ConnectionPool::getIdleConnections() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return idleConnections.size();
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (isShutdown) {
        return;
    }
    
    LOG_INFO("关闭连接池");
    isShutdown = true;
    
    // 关闭所有空闲连接
    while (!idleConnections.empty()) {
        auto conn = idleConnections.front();
        idleConnections.pop();
        conn->close();
    }
    
    // 通知所有等待的线程
    connectionAvailable.notify_all();
}

} // namespace utils