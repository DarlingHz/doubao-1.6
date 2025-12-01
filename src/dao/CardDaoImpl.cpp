#include "include/dao/CardDao.h"
#include "include/db/ConnectionPool.h"
#include "include/model/Card.h"
#include "include/model/RequestResponse.h"
#include <mysql/mysql.h>
#include <chrono>
#include <ctime>
#include <sstream>

namespace dao {

class CardDaoImpl : public CardDao {
public:
    std::shared_ptr<model::Card> findById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT c.id, c.user_id, c.title, c.content, c.is_pinned, c.created_at, c.updated_at, "
                         "GROUP_CONCAT(t.name SEPARATOR ',') as tags "
                         "FROM cards c "
                         "LEFT JOIN card_tags ct ON c.id = ct.card_id "
                         "LEFT JOIN tags t ON ct.tag_id = t.id "
                         "WHERE c.id = " + std::to_string(id) + " "
                         "GROUP BY c.id";
        
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
        std::shared_ptr<model::Card> card = parseCardRow(row);
        
        pool->releaseConnection(conn);
        return card;
    }
    
    std::shared_ptr<model::Card> findByUserIdAndCardId(int userId, int cardId) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT c.id, c.user_id, c.title, c.content, c.is_pinned, c.created_at, c.updated_at, "
                         "GROUP_CONCAT(t.name SEPARATOR ',') as tags "
                         "FROM cards c "
                         "LEFT JOIN card_tags ct ON c.id = ct.card_id "
                         "LEFT JOIN tags t ON ct.tag_id = t.id "
                         "WHERE c.id = " + std::to_string(cardId) + " AND c.user_id = " + std::to_string(userId) + " "
                         "GROUP BY c.id";
        
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
        std::shared_ptr<model::Card> card = parseCardRow(row);
        
        pool->releaseConnection(conn);
        return card;
    }
    
    int create(const model::Card& card) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 开启事务
        conn->execute("START TRANSACTION");
        
        std::string createdAtStr = formatTime(card.getCreatedAt());
        std::string updatedAtStr = formatTime(card.getUpdatedAt());
        
        // 插入卡片
        std::string sql = "INSERT INTO cards (user_id, title, content, is_pinned, created_at, updated_at) VALUES ("
            + std::to_string(card.getUserId()) + ", "
            "'" + escapeString(card.getTitle()) + "', "
            "'" + escapeString(card.getContent()) + "', "
            (card.isPinned() ? "1" : "0") + ", "
            "'" + createdAtStr + "', "
            "'" + updatedAtStr + "'")";
        
        bool success = conn->execute(sql);
        int cardId = success ? conn->lastInsertId() : 0;
        
        if (success && cardId > 0 && !card.getTags().empty()) {
            // 处理标签
            for (const auto& tagName : card.getTags()) {
                // 查找或创建标签
                std::string tagSql = "INSERT INTO tags (user_id, name) VALUES (" 
                    + std::to_string(card.getUserId()) + ", '" + escapeString(tagName) + "') "
                    "ON DUPLICATE KEY UPDATE id = id";
                
                if (!conn->execute(tagSql)) {
                    success = false;
                    break;
                }
                
                // 获取标签ID
                auto tagResult = conn->query("SELECT id FROM tags WHERE user_id = " + std::to_string(card.getUserId()) + 
                                           " AND name = '" + escapeString(tagName) + "'");
                
                if (tagResult) {
                    MYSQL_RES* mysqlTagResult = static_cast<MYSQL_RES*>(tagResult.get());
                    if (mysql_num_rows(mysqlTagResult) > 0) {
                        MYSQL_ROW tagRow = mysql_fetch_row(mysqlTagResult);
                        int tagId = std::stoi(tagRow[0]);
                        
                        // 插入卡片-标签关联
                        std::string relationSql = "INSERT INTO card_tags (card_id, tag_id) VALUES (" 
                            + std::to_string(cardId) + ", " + std::to_string(tagId) + ")";
                        
                        if (!conn->execute(relationSql)) {
                            success = false;
                            break;
                        }
                    }
                }
            }
        }
        
        // 提交或回滚事务
        if (success) {
            conn->execute("COMMIT");
        } else {
            conn->execute("ROLLBACK");
            cardId = 0;
        }
        
        pool->releaseConnection(conn);
        return cardId;
    }
    
    bool update(const model::Card& card) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 开启事务
        conn->execute("START TRANSACTION");
        
        std::string updatedAtStr = formatTime(card.getUpdatedAt());
        
        // 更新卡片基本信息
        std::string sql = "UPDATE cards SET "
            "title = '" + escapeString(card.getTitle()) + "', "
            "content = '" + escapeString(card.getContent()) + "', "
            "is_pinned = " + (card.isPinned() ? "1" : "0") + ", "
            "updated_at = '" + updatedAtStr + "' "
            "WHERE id = " + std::to_string(card.getId()) + " AND user_id = " + std::to_string(card.getUserId());
        
        bool success = conn->execute(sql);
        
        if (success) {
            // 删除旧的标签关联
            if (!conn->execute("DELETE FROM card_tags WHERE card_id = " + std::to_string(card.getId()))) {
                success = false;
            }
            
            // 添加新的标签关联
            if (success && !card.getTags().empty()) {
                for (const auto& tagName : card.getTags()) {
                    // 查找或创建标签
                    std::string tagSql = "INSERT INTO tags (user_id, name) VALUES (" 
                        + std::to_string(card.getUserId()) + ", '" + escapeString(tagName) + "') "
                        "ON DUPLICATE KEY UPDATE id = id";
                    
                    if (!conn->execute(tagSql)) {
                        success = false;
                        break;
                    }
                    
                    // 获取标签ID
                    auto tagResult = conn->query("SELECT id FROM tags WHERE user_id = " + std::to_string(card.getUserId()) + 
                                               " AND name = '" + escapeString(tagName) + "'");
                    
                    if (tagResult) {
                        MYSQL_RES* mysqlTagResult = static_cast<MYSQL_RES*>(tagResult.get());
                        if (mysql_num_rows(mysqlTagResult) > 0) {
                            MYSQL_ROW tagRow = mysql_fetch_row(mysqlTagResult);
                            int tagId = std::stoi(tagRow[0]);
                            
                            // 插入卡片-标签关联
                            std::string relationSql = "INSERT INTO card_tags (card_id, tag_id) VALUES (" 
                                + std::to_string(card.getId()) + ", " + std::to_string(tagId) + ")";
                            
                            if (!conn->execute(relationSql)) {
                                success = false;
                                break;
                            }
                        }
                    }
                }
            }
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
    
    bool deleteById(int id) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 开启事务
        conn->execute("START TRANSACTION");
        
        // 删除标签关联
        bool success = conn->execute("DELETE FROM card_tags WHERE card_id = " + std::to_string(id));
        
        // 删除卡片
        if (success) {
            success = conn->execute("DELETE FROM cards WHERE id = " + std::to_string(id));
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
    
    model::PaginatedResponse<model::Card> findByUserId(int userId, const model::CardListQuery& query) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 构建排序部分
        std::string sortBy = query.sort_by == "created_at" ? "created_at" : "updated_at";
        std::string sortOrder = query.sort_order == "asc" ? "ASC" : "DESC";
        
        // 计算偏移量
        int offset = (query.page - 1) * query.page_size;
        
        // 查询总数
        std::string countSql = "SELECT COUNT(*) FROM cards WHERE user_id = " + std::to_string(userId);
        auto countResult = conn->query(countSql);
        int total = 0;
        
        if (countResult) {
            MYSQL_RES* mysqlCountResult = static_cast<MYSQL_RES*>(countResult.get());
            if (mysql_num_rows(mysqlCountResult) > 0) {
                MYSQL_ROW countRow = mysql_fetch_row(mysqlCountResult);
                total = std::stoi(countRow[0]);
            }
        }
        
        // 查询卡片列表
        std::string sql = "SELECT c.id, c.user_id, c.title, c.content, c.is_pinned, c.created_at, c.updated_at, "
                         "GROUP_CONCAT(t.name SEPARATOR ',') as tags "
                         "FROM cards c "
                         "LEFT JOIN card_tags ct ON c.id = ct.card_id "
                         "LEFT JOIN tags t ON ct.tag_id = t.id "
                         "WHERE c.user_id = " + std::to_string(userId) + " "
                         "GROUP BY c.id "
                         "ORDER BY c." + sortBy + " " + sortOrder + " "
                         "LIMIT " + std::to_string(query.page_size) + " OFFSET " + std::to_string(offset);
        
        auto result = conn->query(sql);
        std::vector<model::Card> cards;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            MYSQL_ROW row;
            
            while ((row = mysql_fetch_row(mysqlResult)) != nullptr) {
                auto card = parseCardRow(row);
                if (card) {
                    cards.push_back(*card);
                }
            }
        }
        
        pool->releaseConnection(conn);
        
        model::PaginatedResponse<model::Card> response;
        response.total = total;
        response.page = query.page;
        response.page_size = query.page_size;
        response.items = cards;
        
        return response;
    }
    
    std::vector<std::shared_ptr<model::Card>> findByTags(int userId, const std::vector<std::string>& tags) override {
        // 实现按标签查询的逻辑
        // 这里简化实现，实际应该使用子查询或连接查询
        return {};
    }
    
    model::PaginatedResponse<model::Card> search(int userId, const std::string& keyword, const model::CardListQuery& query) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        // 构建排序部分
        std::string sortBy = query.sort_by == "created_at" ? "created_at" : "updated_at";
        std::string sortOrder = query.sort_order == "asc" ? "ASC" : "DESC";
        
        // 计算偏移量
        int offset = (query.page - 1) * query.page_size;
        
        // 构建搜索条件
        std::string searchCondition = "WHERE c.user_id = " + std::to_string(userId) + 
                                     " AND (c.title LIKE '%" + escapeString(keyword) + "%' OR c.content LIKE '%" + escapeString(keyword) + "%')";
        
        // 查询总数
        std::string countSql = "SELECT COUNT(*) FROM cards c " + searchCondition;
        auto countResult = conn->query(countSql);
        int total = 0;
        
        if (countResult) {
            MYSQL_RES* mysqlCountResult = static_cast<MYSQL_RES*>(countResult.get());
            if (mysql_num_rows(mysqlCountResult) > 0) {
                MYSQL_ROW countRow = mysql_fetch_row(mysqlCountResult);
                total = std::stoi(countRow[0]);
            }
        }
        
        // 查询卡片列表
        std::string sql = "SELECT c.id, c.user_id, c.title, c.content, c.is_pinned, c.created_at, c.updated_at, "
                         "GROUP_CONCAT(t.name SEPARATOR ',') as tags "
                         "FROM cards c "
                         "LEFT JOIN card_tags ct ON c.id = ct.card_id "
                         "LEFT JOIN tags t ON ct.tag_id = t.id "
                         + searchCondition + " "
                         "GROUP BY c.id "
                         "ORDER BY c." + sortBy + " " + sortOrder + " "
                         "LIMIT " + std::to_string(query.page_size) + " OFFSET " + std::to_string(offset);
        
        auto result = conn->query(sql);
        std::vector<model::Card> cards;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            MYSQL_ROW row;
            
            while ((row = mysql_fetch_row(mysqlResult)) != nullptr) {
                auto card = parseCardRow(row);
                if (card) {
                    cards.push_back(*card);
                }
            }
        }
        
        pool->releaseConnection(conn);
        
        model::PaginatedResponse<model::Card> response;
        response.total = total;
        response.page = query.page;
        response.page_size = query.page_size;
        response.items = cards;
        
        return response;
    }
    
    int countByUserId(int userId) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT COUNT(*) FROM cards WHERE user_id = " + std::to_string(userId);
        auto result = conn->query(sql);
        int count = 0;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            if (mysql_num_rows(mysqlResult) > 0) {
                MYSQL_ROW row = mysql_fetch_row(mysqlResult);
                count = std::stoi(row[0]);
            }
        }
        
        pool->releaseConnection(conn);
        return count;
    }
    
    std::vector<std::pair<std::string, int>> getRecentDailyCount(int userId, int days) override {
        auto pool = db::ConnectionPool::getInstance();
        auto conn = pool->getConnection();
        
        std::string sql = "SELECT DATE(created_at) as date, COUNT(*) as count "
                         "FROM cards WHERE user_id = " + std::to_string(userId) + " "
                         "AND created_at >= DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY) "
                         "GROUP BY DATE(created_at) "
                         "ORDER BY date";
        
        auto result = conn->query(sql);
        std::vector<std::pair<std::string, int>> dailyCounts;
        
        if (result) {
            MYSQL_RES* mysqlResult = static_cast<MYSQL_RES*>(result.get());
            MYSQL_ROW row;
            
            while ((row = mysql_fetch_row(mysqlResult)) != nullptr) {
                dailyCounts.push_back({row[0], std::stoi(row[1])});
            }
        }
        
        pool->releaseConnection(conn);
        return dailyCounts;
    }
    
private:
    std::shared_ptr<model::Card> parseCardRow(MYSQL_ROW row) {
        if (!row) return nullptr;
        
        // 解析时间
        auto parseTime = [](const char* timeStr) {
            struct tm tm_time;
            strptime(timeStr, "%Y-%m-%d %H:%M:%S", &tm_time);
            return std::chrono::system_clock::from_time_t(mktime(&tm_time));
        };
        
        // 解析标签
        std::vector<std::string> tags;
        if (row[7] && strlen(row[7]) > 0) {
            std::stringstream ss(row[7]);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                tags.push_back(tag);
            }
        }
        
        return std::make_shared<model::Card>(
            std::stoi(row[0]),
            std::stoi(row[1]),
            row[2] ? row[2] : "",
            row[3] ? row[3] : "",
            tags,
            std::stoi(row[4]) == 1,
            parseTime(row[5]),
            parseTime(row[6])
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