#include "repository/Database.h"
#include <stdexcept>
#include <iostream>
#include <regex>

namespace repository {

#ifdef USE_MOCK_DATABASE

MockResult::MockResult(int rows, int cols) : rowCount(rows), colCount(cols) {
    data.resize(rows, std::vector<std::string>(cols, ""));
}

MockResult::~MockResult() {
}

void MockResult::setValue(int row, int col, const std::string& value) {
    if (row >= 0 && row < rowCount && col >= 0 && col < colCount) {
        data[row][col] = value;
    }
}

std::string MockResult::getValue(int row, int col) const {
    if (row >= 0 && row < rowCount && col >= 0 && col < colCount) {
        return data[row][col];
    }
    return "";
}

int MockResult::getRowCount() const {
    return rowCount;
}

int MockResult::getColumnCount() const {
    return colCount;
}

int MockResult::getStatus() const {
    return 1; // 模拟成功状态
}

#endif

Database::~Database() {
    close();
}

Database::Database(Database&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        close();
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

#ifdef USE_MOCK_DATABASE

// 构造函数已在头文件中定义为默认构造函数

bool Database::open(const std::string& db_path) {
    std::cout << "模拟打开数据库: " << db_path << std::endl;
    dbPath_ = db_path;
    db_ = reinterpret_cast<void*>(1); // 非空指针，表示已连接
    return true;
}

void Database::close() {
    if (db_) {
        std::cout << "模拟关闭数据库" << std::endl;
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    std::cout << "模拟执行SQL: " << sql << std::endl;
    return true;
}

std::string Database::queryScalar(const std::string& sql) {
    std::cout << "模拟查询标量: " << sql << std::endl;
    // 简单的模拟逻辑，根据SQL返回不同的值
    if (sql.find("COUNT") != std::string::npos) {
        return "5";
    }
    return "mock_value";
}

bool Database::query(const std::string& sql, int (*callback)(void*, int, char**, char**), void* data) {
    std::cout << "模拟执行查询: " << sql << std::endl;
    // 简单模拟回调调用
    if (callback) {
        char* columns[] = {(char*)"id", (char*)"name"};
        char* values[] = {(char*)"1", (char*)"mock_item"};
        callback(data, 2, values, columns);
    }
    return true;
}

int64_t Database::getLastInsertId() const {
    return 1; // 模拟ID
}

std::string Database::getError() const {
    return "";
}

// isValid方法已在头文件中内联定义

void Database::initializeMockData() {
    // 初始化模拟用户数据
    std::map<std::string, std::string> user1;
    user1["id"] = "1";
    user1["username"] = "admin";
    user1["password"] = "admin123";
    user1["nickname"] = "管理员";
    mockUsers.push_back(user1);
    
    std::map<std::string, std::string> user2;
    user2["id"] = "2";
    user2["username"] = "user1";
    user2["password"] = "password1";
    user2["nickname"] = "用户一";
    mockUsers.push_back(user2);
    
    // 初始化模拟电影数据
    std::map<std::string, std::string> movie1;
    movie1["id"] = "1";
    movie1["title"] = "肖申克的救赎";
    movie1["description"] = "银行家安迪因被误判谋杀妻子及其情人而入狱，在肖申克监狱中寻找希望和自由。";
    movie1["genres"] = "{剧情,犯罪}";
    movie1["duration"] = "142";
    movie1["release_year"] = "1994";
    mockMovies.push_back(movie1);
    
    std::map<std::string, std::string> movie2;
    movie2["id"] = "2";
    movie2["title"] = "阿甘正传";
    movie2["description"] = "一个智商只有75的男孩阿甘，通过自己的努力和坚持，在人生中创造了奇迹。";
    movie2["genres"] = "{剧情,爱情}";
    movie2["duration"] = "142";
    movie2["release_year"] = "1994";
    mockMovies.push_back(movie2);
}

MockResultPtr Database::mockExecuteQuery(const std::string& query) {
    return createMockResultForQuery(query);
}

MockResultPtr Database::createMockResultForQuery(const std::string& query) {
    // 简单的SQL解析，根据查询的表名返回相应的模拟数据
    std::regex usersRegex("SELECT.*FROM.*users", std::regex::icase);
    std::regex moviesRegex("SELECT.*FROM.*movies", std::regex::icase);
    
    if (std::regex_search(query, usersRegex)) {
        // 返回用户数据
        auto result = std::make_shared<MockResult>(mockUsers.size(), 4);
        for (size_t i = 0; i < mockUsers.size(); ++i) {
            result->setValue(i, 0, mockUsers[i]["id"]);
            result->setValue(i, 1, mockUsers[i]["username"]);
            result->setValue(i, 2, mockUsers[i]["nickname"]);
            result->setValue(i, 3, mockUsers[i]["password"]);
        }
        return result;
    } else if (std::regex_search(query, moviesRegex)) {
        // 返回电影数据
        auto result = std::make_shared<MockResult>(mockMovies.size(), 6);
        for (size_t i = 0; i < mockMovies.size(); ++i) {
            result->setValue(i, 0, mockMovies[i]["id"]);
            result->setValue(i, 1, mockMovies[i]["title"]);
            result->setValue(i, 2, mockMovies[i]["description"]);
            result->setValue(i, 3, mockMovies[i]["genres"]);
            result->setValue(i, 4, mockMovies[i]["duration"]);
            result->setValue(i, 5, mockMovies[i]["release_year"]);
        }
        return result;
    }
    
    // 默认返回空结果
    return std::make_shared<MockResult>(0, 0);
}

#else

bool Database::open(const std::string& db_path) {
    // 实际实现留空，因为我们使用模拟数据库
    return false;
}

void Database::close() {
    // 实际实现留空
    db_ = nullptr;
}

bool Database::execute(const std::string& sql) {
    return false;
}

std::string Database::queryScalar(const std::string& sql) {
    return "";
}

bool Database::query(const std::string& sql, QueryCallback callback, void* data) {
    return false;
}

int64_t Database::getLastInsertId() const {
    return 0;
}

std::string Database::getError() const {
    return "数据库未打开";
}

#endif

} // namespace repository
