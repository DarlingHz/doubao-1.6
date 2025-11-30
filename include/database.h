#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <memory>
#include <vector>
#include <map>

// SQLite 错误码处理
#define SQLITE_OK 0

// 数据库连接管理类 - 使用单例模式
class Database {
public:
    // 获取数据库实例
    static Database& getInstance();
    
    // 禁止拷贝和移动
    Database(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(const Database&) = delete;
    Database& operator=(Database&&) = delete;
    
    // 连接数据库
    bool connect(const std::string& dbPath);
    
    // 断开连接
    void disconnect();
    
    // 执行 SQL 语句（不返回结果）
    bool execute(const std::string& sql);
    
    // 执行 SQL 查询并返回结果集
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    
    // 开始事务
    bool beginTransaction();
    
    // 提交事务
    bool commitTransaction();
    
    // 回滚事务
    bool rollbackTransaction();
    
    // 获取最后一次插入的 ID
    int64_t getLastInsertId();
    
    // 获取 SQLite 错误信息
    std::string getErrorMessage() const;
    
    // 获取原始数据库连接
    sqlite3* getConnection();
    
private:
    // 私有构造函数
    Database();
    
    // 析构函数
    ~Database();
    
    // 数据库连接句柄
    sqlite3* db_;
    
    // 错误信息
    std::string errorMessage_;
};

// 预处理语句类 - 优化数据库查询性能
class PreparedStatement {
public:
    PreparedStatement(sqlite3* db, const std::string& sql);
    ~PreparedStatement();
    
    // 绑定参数
    bool bind(int index, const std::string& value);
    bool bind(int index, int value);
    bool bind(int index, int64_t value);
    bool bind(int index, double value);
    
    // 执行查询
    bool execute();
    
    // 获取结果
    bool next();
    
    // 获取字段值
    std::string getString(int column);
    int getInt(int column);
    int64_t getInt64(int column);
    double getDouble(int column);
    
    // 获取错误信息
    std::string getErrorMessage() const;
    
private:
    sqlite3_stmt* stmt_;
    std::string errorMessage_;
};

#endif // DATABASE_H
