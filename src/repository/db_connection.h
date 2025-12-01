// 数据库连接管理
#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <sqlite3.h>
#include <string>
#include <mutex>
#include <memory>

class DBConnection {
private:
    sqlite3* db;
    std::mutex dbMutex;  // 用于保护数据库操作的互斥锁
    
public:
    DBConnection();
    ~DBConnection();
    
    // 打开数据库连接
    bool open(const std::string& dbPath);
    
    // 关闭数据库连接
    void close();
    
    // 执行SQL语句（不返回结果）
    bool execute(const std::string& sql);
    
    // 执行查询并回调处理结果
    bool executeQuery(const std::string& sql, int (*callback)(void*, int, char**, char**), void* data);
    
    // 获取数据库指针（使用时需注意线程安全）
    sqlite3* getDB() { return db; }
    
    // 获取互斥锁
    std::mutex& getMutex() { return dbMutex; }
    
    // 初始化数据库表结构
    bool initializeTables();
};

// 数据库连接单例
class DBManager {
private:
    static DBManager* instance;
    std::unique_ptr<DBConnection> connection;
    
    DBManager();
    
public:
    static DBManager* getInstance();
    
    // 获取数据库连接
    DBConnection* getConnection();
    
    // 初始化数据库
    bool initialize();
};

#endif // DB_CONNECTION_H
