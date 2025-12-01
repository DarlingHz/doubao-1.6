#include "include/dao/UserDao.h"
#include "include/db/ConnectionPool.h"
#include "include/model/User.h"
#include <mysql/mysql.h>
#include <chrono>
#include <ctime>

namespace dao {

class UserDaoImpl : public UserDao {
public:
    std::shared_ptr<model::User> findById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT id, email, password_hash, salt, created_at, updated_at FROM users WHERE id = " + std::to_string(id);
        auto result = conn->query(sql);
        
        if (!result) {
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
        if (mysql_num_rows(mysqlResult) == 0) {
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        MYSQL_ROW row = mysql_fetch_row(mysqlResult);
        std::shared_ptr<model::User> user = parseUserRow(row);
        
        pool->releaseConnection(conn);
        return user;
    }
    
    std::shared_ptr<model::User> findByEmail(const std::string& email) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 使用预处理语句防止SQL注入
        MYSQL* mysqlConn = static_cast<MYSQL*>(conn->query("").get());
        MYSQL_STMT* stmt = mysql_stmt_init(mysqlConn);
        
        const char* sql = "SELECT id, email, password_hash, salt, created_at, updated_at FROM users WHERE email = ?";
        if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
            mysql_stmt_close(stmt);
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        MYSQL_BIND bind[1];
        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = const_cast<char*>(email.c_str());
        bind[0].buffer_length = email.length();
        
        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            mysql_stmt_close(stmt);
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        if (mysql_stmt_execute(stmt) != 0) {
            mysql_stmt_close(stmt);
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        // 获取结果
        MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
        if (!result) {
            mysql_stmt_close(stmt);
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        int id = 0;
        char emailBuffer[255] = {0};
        char passwordHash[255] = {0};
        char salt[255] = {0};
        char createdAt[20] = {0};
        char updatedAt[20] = {0};
        
        unsigned long lengths[6];
        my_bool isNull[6] = {0};
        
        MYSQL_BIND resultBind[6];
        memset(resultBind, 0, sizeof(resultBind));
        
        resultBind[0].buffer_type = MYSQL_TYPE_LONG;
        resultBind[0].buffer = &id;
        resultBind[0].is_null = &isNull[0];
        
        resultBind[1].buffer_type = MYSQL_TYPE_STRING;
        resultBind[1].buffer = emailBuffer;
        resultBind[1].buffer_length = sizeof(emailBuffer);
        resultBind[1].length = &lengths[1];
        resultBind[1].is_null = &isNull[1];
        
        resultBind[2].buffer_type = MYSQL_TYPE_STRING;
        resultBind[2].buffer = passwordHash;
        resultBind[2].buffer_length = sizeof(passwordHash);
        resultBind[2].length = &lengths[2];
        resultBind[2].is_null = &isNull[2];
        
        resultBind[3].buffer_type = MYSQL_TYPE_STRING;
        resultBind[3].buffer = salt;
        resultBind[3].buffer_length = sizeof(salt);
        resultBind[3].length = &lengths[3];
        resultBind[3].is_null = &isNull[3];
        
        resultBind[4].buffer_type = MYSQL_TYPE_STRING;
        resultBind[4].buffer = createdAt;
        resultBind[4].buffer_length = sizeof(createdAt);
        resultBind[4].length = &lengths[4];
        resultBind[4].is_null = &isNull[4];
        
        resultBind[5].buffer_type = MYSQL_TYPE_STRING;
        resultBind[5].buffer = updatedAt;
        resultBind[5].buffer_length = sizeof(updatedAt);
        resultBind[5].length = &lengths[5];
        resultBind[5].is_null = &isNull[5];
        
        if (mysql_stmt_bind_result(stmt, resultBind) != 0) {
            mysql_free_result(result);
            mysql_stmt_close(stmt);
            pool->releaseConnection(conn);
            return nullptr;
        }
        
        std::shared_ptr<model::User> user = nullptr;
        if (mysql_stmt_fetch(stmt) == 0) {
            // 解析时间
            auto parseTime = [](const char* timeStr) {
                struct tm tm_time;
                strptime(timeStr, "%Y-%m-%d %H:%M:%S", &tm_time);
                return std::chrono::system_clock::from_time_t(mktime(&tm_time));
            };
            
            user = std::make_shared<model::User>(
                id,
                std::string(emailBuffer),
                std::string(passwordHash),
                std::string(salt),
                parseTime(createdAt),
                parseTime(updatedAt)
            );
        }
        
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        pool->releaseConnection(conn);
        
        return user;
    }
    
    int create(const model::User& user) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string createdAtStr = formatTime(user.getCreatedAt());
        std::string updatedAtStr = formatTime(user.getUpdatedAt());
        
        std::string sql = "INSERT INTO users (email, password_hash, salt, created_at, updated_at) VALUES (" 
            "'" + escapeString(user.getEmail()) + "', "
            "'" + escapeString(user.getPasswordHash()) + "', "
            "'" + escapeString(user.getSalt()) + "', "
            "'" + createdAtStr + "', "
            "'" + updatedAtStr + "'")";
        
        bool success = conn->execute(sql);
        int id = success ? conn->lastInsertId() : 0;
        
        pool->releaseConnection(conn);
        return id;
    }
    
    bool update(const model::User& user) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string updatedAtStr = formatTime(user.getUpdatedAt());
        
        std::string sql = "UPDATE users SET "
            "email = '" + escapeString(user.getEmail()) + "', "
            "password_hash = '" + escapeString(user.getPasswordHash()) + "', "
            "salt = '" + escapeString(user.getSalt()) + "', "
            "updated_at = '" + updatedAtStr + "' "
            "WHERE id = " + std::to_string(user.getId());
        
        bool success = conn->execute(sql);
        pool->releaseConnection(conn);
        return success;
    }
    
    bool deleteById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "DELETE FROM users WHERE id = " + std::to_string(id);
        bool success = conn->execute(sql);
        
        pool->releaseConnection(conn);
        return success;
    }
    
private:
    std::shared_ptr<model::User> parseUserRow(MYSQL_ROW row) {
        if (!row) return nullptr;
        
        // 解析时间
        auto parseTime = [](const char* timeStr) {
            struct tm tm_time;
            strptime(timeStr, "%Y-%m-%d %H:%M:%S", &tm_time);
            return std::chrono::system_clock::from_time_t(mktime(&tm_time));
        };
        
        return std::make_shared<model::User>(
            std::stoi(row[0]),
            row[1] ? row[1] : "",
            row[2] ? row[2] : "",
            row[3] ? row[3] : "",
            parseTime(row[4]),
            parseTime(row[5])
        );
    }
    
    std::string formatTime(const std::chrono::system_clock::time_point& timePoint) {
        auto time = std::chrono::system_clock::to_time_t(timePoint);
        struct tm* tm_time = std::localtime(&time);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_time);
        return std::string(buffer);
    }
    
    std::string escapeString(const std::string& str) {
        // 简单的SQL字符串转义
        std::string result;
        for (char c : str) {
            if (c == '\'') {
                result += "\\'";
            } else if (c == '\\') {
                result += "\\\\";
            } else {
                result += c;
            }
        }
        return result;
    }
};

} // namespace dao