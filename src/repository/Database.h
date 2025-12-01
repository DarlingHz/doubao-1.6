#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace repository {

#ifdef USE_MOCK_DATABASE

// 模拟查询结果类
class MockResult {
private:
    int rowCount;
    int colCount;
    std::vector<std::vector<std::string>> data;

public:
    MockResult(int rows, int cols);
    ~MockResult();
    
    void setValue(int row, int col, const std::string& value);
    std::string getValue(int row, int col) const;
    int getRowCount() const;
    int getColumnCount() const;
    int getStatus() const;
};

using MockResultPtr = std::shared_ptr<MockResult>;

class Database {
private:
    std::string dbPath_;
    void* db_ = nullptr; // 使用void*代替具体的数据库类型
    
    // 模拟数据存储
    std::vector<std::map<std::string, std::string>> mockUsers;
    std::vector<std::map<std::string, std::string>> mockMovies;
    std::vector<std::map<std::string, std::string>> mockWatchRecords;
    
    // 初始化模拟数据
    void initializeMockData();
    // 模拟执行查询
    MockResultPtr mockExecuteQuery(const std::string& query);
    // 为特定查询创建模拟结果
    MockResultPtr createMockResultForQuery(const std::string& query);

public:
    Database() = default;
    ~Database();
    
    // 禁用拷贝构造和赋值操作
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // 支持移动构造和移动赋值
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;
    
    // 打开数据库连接
    bool open(const std::string& db_path);
    
    // 关闭数据库连接
    void close();
    
    // 执行SQL语句
    bool execute(const std::string& sql);
    
    // 执行查询并返回单值结果
    std::string queryScalar(const std::string& sql);
    
    // 执行查询并使用回调函数处理结果
    bool query(const std::string& sql, int (*callback)(void*, int, char**, char**) = nullptr, void* data = nullptr);
    
    // 获取最后插入的ID
    int64_t getLastInsertId() const;
    
    // 获取错误信息
    std::string getError() const;
    
    // 检查连接是否有效
    bool isValid() const { return db_ != nullptr; }
    
    // 获取原始数据库连接
    void* getHandle() { return db_; }
    

};

#else

class Database {
private:
    std::string dbPath_;
    void* db_ = nullptr; // 使用void*代替具体的数据库类型

public:
    Database() = default;
    ~Database();
    
    // 禁用拷贝构造和赋值操作
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // 支持移动构造和移动赋值
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;
    
    // 打开数据库连接
    bool open(const std::string& db_path);
    
    // 关闭数据库连接
    void close();
    
    // 执行SQL语句（无返回值）
    bool execute(const std::string& sql);
    
    // 执行查询并返回单值结果
    std::string queryScalar(const std::string& sql);
    
    // 执行查询并使用回调函数处理结果
    using QueryCallback = int (*)(void*, int, char**, char**);
    bool query(const std::string& sql, QueryCallback callback, void* data = nullptr);
    
    // 获取最后一个插入的ID
    int64_t getLastInsertId() const;
    
    // 获取错误信息
    std::string getError() const;
    
    // 检查连接是否有效
    bool isValid() const { return db_ != nullptr; }
    
    // 获取原始数据库连接
    void* getHandle() { return db_; }
};

#endif

// 数据库连接智能指针
using DatabasePtr = std::shared_ptr<Database>;

} // namespace repository

#endif // DATABASE_H
