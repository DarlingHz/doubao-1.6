#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <atomic>

namespace utils {

class Connection {
public:
    using Ptr = std::shared_ptr<Connection>;
    
    Connection(const std::string& host, int port, const std::string& dbname, 
               const std::string& user, const std::string& password);
    ~Connection();
    
    bool isConnected() const;
    void* getNativeHandle();
    void close();
    
private:
    void* connectionHandle; // 实际的数据库连接句柄
    bool connected;
};

class ConnectionPool {
public:
    using ConnectionCreator = std::function<Connection::Ptr()>;
    
    ConnectionPool(size_t minConnections, size_t maxConnections,
                  const std::string& host, int port, const std::string& dbname,
                  const std::string& user, const std::string& password);
    
    ~ConnectionPool();
    
    Connection::Ptr getConnection(int timeoutMs = 5000);
    void returnConnection(Connection::Ptr connection);
    
    size_t getActiveConnections() const;
    size_t getIdleConnections() const;
    
    void shutdown();
    
private:
    void initializeConnections();
    Connection::Ptr createConnection();
    
    size_t minConnections;
    size_t maxConnections;
    std::string host;
    int port;
    std::string dbname;
    std::string user;
    std::string password;
    
    std::queue<Connection::Ptr> idleConnections;
    std::atomic<size_t> activeConnectionCount;
    
    mutable std::mutex poolMutex;
    std::condition_variable connectionAvailable;
    bool isShutdown;
};

} // namespace utils

#endif // CONNECTIONPOOL_H