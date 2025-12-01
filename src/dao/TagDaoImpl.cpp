#include "include/dao/TagDao.h"
#include "include/db/ConnectionPool.h"
#include "include/model/Tag.h"
#include <mysql/mysql.h>

namespace dao {

class TagDaoImpl : public TagDao {
public:
    std::shared_ptr<model::Tag> findById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT id, user_id, name FROM tags WHERE id = " + std::to_string(id);
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
        std::shared_ptr<model::Tag> tag = parseTagRow(row);
        
        pool->releaseConnection(conn);
        return tag;
    }
    
    std::shared_ptr<model::Tag> findByUserIdAndName(int userId, const std::string& name) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT id, user_id, name FROM tags WHERE user_id = " + std::to_string(userId) + 
                         " AND name = '" + escapeString(name) + "'";
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
        std::shared_ptr<model::Tag> tag = parseTagRow(row);
        
        pool->releaseConnection(conn);
        return tag;
    }
    
    std::vector<std::shared_ptr<model::Tag>> findAllByUserId(int userId) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 查询标签及其卡片数量
        std::string sql = "SELECT t.id, t.user_id, t.name, COUNT(ct.card_id) as card_count "
                         "FROM tags t "
                         "LEFT JOIN card_tags ct ON t.id = ct.tag_id "
                         "WHERE t.user_id = " + std::to_string(userId) + " "
                         "GROUP BY t.id "
                         "ORDER BY card_count DESC";
        
        auto result = conn->query(sql);
        std::vector<std::shared_ptr<model::Tag>> tags;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            MYSQL_ROW row;
            
            while ((row = mysql_fetch_row(mysqlResult)) != nullptr) {
                if (row[0] && row[1] && row[2]) {
                    int id = std::stoi(row[0]);
                    int userId = std::stoi(row[1]);
                    std::string name = row[2];
                    int cardCount = row[3] ? std::stoi(row[3]) : 0;
                    
                    tags.push_back(std::make_shared<model::Tag>(id, userId, name, cardCount));
                }
            }
        }
        
        pool->releaseConnection(conn);
        return tags;
    }
    
    int create(const model::Tag& tag) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "INSERT INTO tags (user_id, name) VALUES (" 
            + std::to_string(tag.getUserId()) + ", '" + escapeString(tag.getName()) + "')";
        
        bool success = conn->execute(sql);
        int id = success ? conn->lastInsertId() : 0;
        
        pool->releaseConnection(conn);
        return id;
    }
    
    bool update(const model::Tag& tag) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "UPDATE tags SET name = '" + escapeString(tag.getName()) + "' "
                         "WHERE id = " + std::to_string(tag.getId()) + " AND user_id = " + std::to_string(tag.getUserId());
        
        bool success = conn->execute(sql);
        pool->releaseConnection(conn);
        return success;
    }
    
    bool deleteById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 开启事务
        conn->execute("START TRANSACTION");
        
        // 删除标签关联
        bool success = conn->execute("DELETE FROM card_tags WHERE tag_id = " + std::to_string(id));
        
        // 删除标签
        if (success) {
            success = conn->execute("DELETE FROM tags WHERE id = " + std::to_string(id));
        }
        
        // 提交或回滚事务
        if (success) {
            conn->execute("COMMIT");
        } else {
            conn->execute("ROLLBACK");
        }
        
        pool->releaseConnection(conn);
        return success;
    }
    
    bool rename(int userId, const std::string& oldName, const std::string& newName) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 检查新标签名是否已存在
        auto existingTag = findByUserIdAndName(userId, newName);
        if (existingTag) {
            pool->releaseConnection(conn);
            return false; // 新标签名已存在
        }
        
        std::string sql = "UPDATE tags SET name = '" + escapeString(newName) + "' "
                         "WHERE user_id = " + std::to_string(userId) + " AND name = '" + escapeString(oldName) + "'";
        
        bool success = conn->execute(sql);
        pool->releaseConnection(conn);
        return success;
    }
    
    bool merge(int userId, const std::string& tagToMerge, const std::string& targetTag) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 开启事务
        conn->execute("START TRANSACTION");
        
        // 获取要合并的标签ID
        auto mergeTag = findByUserIdAndName(userId, tagToMerge);
        auto targetTagObj = findByUserIdAndName(userId, targetTag);
        
        if (!mergeTag || !targetTagObj) {
            conn->execute("ROLLBACK");
            pool->releaseConnection(conn);
            return false;
        }
        
        int mergeTagId = mergeTag->getId();
        int targetTagId = targetTagObj->getId();
        
        // 更新所有关联到mergeTag的卡片，改为关联到targetTag
        bool success = conn->execute(
            "UPDATE card_tags SET tag_id = " + std::to_string(targetTagId) + " "
            "WHERE tag_id = " + std::to_string(mergeTagId)
        );
        
        // 删除被合并的标签
        if (success) {
            success = conn->execute("DELETE FROM tags WHERE id = " + std::to_string(mergeTagId));
        }
        
        // 提交或回滚事务
        if (success) {
            conn->execute("COMMIT");
        } else {
            conn->execute("ROLLBACK");
        }
        
        pool->releaseConnection(conn);
        return success;
    }
    
    std::vector<std::shared_ptr<model::Tag>> findTopTagsByUserId(int userId, int limit) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT t.id, t.user_id, t.name, COUNT(ct.card_id) as card_count "
                         "FROM tags t "
                         "LEFT JOIN card_tags ct ON t.id = ct.tag_id "
                         "WHERE t.user_id = " + std::to_string(userId) + " "
                         "GROUP BY t.id "
                         "ORDER BY card_count DESC "
                         "LIMIT " + std::to_string(limit);
        
        auto result = conn->query(sql);
        std::vector<std::shared_ptr<model::Tag>> tags;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            MYSQL_ROW row;
            
            while ((row = mysql_fetch_row(mysqlResult)) != nullptr) {
                if (row[0] && row[1] && row[2]) {
                    int id = std::stoi(row[0]);
                    int userId = std::stoi(row[1]);
                    std::string name = row[2];
                    int cardCount = row[3] ? std::stoi(row[3]) : 0;
                    
                    tags.push_back(std::make_shared<model::Tag>(id, userId, name, cardCount));
                }
            }
        }
        
        pool->releaseConnection(conn);
        return tags;
    }
    
private:
    std::shared_ptr<model::Tag> parseTagRow(MYSQL_ROW row) {
        if (!row || !row[0] || !row[1] || !row[2]) return nullptr;
        
        return std::make_shared<model::Tag>(
            std::stoi(row[0]),
            std::stoi(row[1]),
            row[2]
        );
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