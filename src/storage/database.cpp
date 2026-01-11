#include "../include/storage/database.h"
#include "../include/utils/logger.h"
#include <string>
#include <cstring>

namespace storage {

Database::Database(const std::string& dbPath) : db_(nullptr), dbPath_(dbPath) {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::initialize() {
    // 打开数据库连接
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        utils::LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    // 创建表
    if (!createTables()) {
        return false;
    }
    
    utils::LOG_INFO("Database initialized successfully: " + dbPath_);
    return true;
}

bool Database::createTables() {
    // 创建 links 表
    const char* createLinksTable = R"(
        CREATE TABLE IF NOT EXISTS links (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            long_url TEXT NOT NULL,
            short_code TEXT NOT NULL UNIQUE,
            custom_alias TEXT UNIQUE,
            created_at INTEGER NOT NULL,
            expire_at INTEGER DEFAULT 0,
            status INTEGER DEFAULT 1,
            visit_count INTEGER DEFAULT 0
        );
    )";
    
    // 创建 visits 表
    const char* createVisitsTable = R"(
        CREATE TABLE IF NOT EXISTS visits (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            link_id INTEGER NOT NULL,
            visit_time INTEGER NOT NULL,
            ip_address TEXT NOT NULL,
            user_agent TEXT,
            FOREIGN KEY (link_id) REFERENCES links(id) ON DELETE CASCADE
        );
    )";
    
    // 创建索引
    const char* createShortCodeIndex = "CREATE INDEX IF NOT EXISTS idx_short_code ON links(short_code);";
    const char* createCustomAliasIndex = "CREATE INDEX IF NOT EXISTS idx_custom_alias ON links(custom_alias);";
    const char* createStatusIndex = "CREATE INDEX IF NOT EXISTS idx_status ON links(status);";
    const char* createLinkIdIndex = "CREATE INDEX IF NOT EXISTS idx_link_id ON visits(link_id);";
    const char* createVisitTimeIndex = "CREATE INDEX IF NOT EXISTS idx_visit_time ON visits(visit_time);";
    
    return executeStatement(createLinksTable) && 
           executeStatement(createVisitsTable) &&
           executeStatement(createShortCodeIndex) &&
           executeStatement(createCustomAliasIndex) &&
           executeStatement(createStatusIndex) &&
           executeStatement(createLinkIdIndex) &&
           executeStatement(createVisitTimeIndex);
}

bool Database::executeStatement(const char* sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        utils::LOG_ERROR("SQL error: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

std::optional<uint64_t> Database::createLink(const model::ShortLink& link) {
    const char* sql = R"(
        INSERT INTO links (long_url, short_code, custom_alias, created_at, expire_at, status, visit_count)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        utils::LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, link.longUrl.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, link.shortCode.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!link.customAlias.empty()) {
        sqlite3_bind_text(stmt, 3, link.customAlias.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    
    sqlite3_bind_int64(stmt, 4, link.createdAt);
    sqlite3_bind_int64(stmt, 5, link.expireAt);
    sqlite3_bind_int(stmt, 6, static_cast<int>(link.status));
    sqlite3_bind_int(stmt, 7, link.visitCount);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        utils::LOG_ERROR("Failed to insert link: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    
    uint64_t lastInsertId = sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    
    utils::LOG_INFO("Link created with ID: " + std::to_string(lastInsertId));
    return lastInsertId;
}

std::optional<model::ShortLink> Database::getLinkById(uint64_t id) {
    const char* sql = "SELECT * FROM links WHERE id = ?";
    return getLinkByStatement(sql, id);
}

std::optional<model::ShortLink> Database::getLinkByShortCode(const std::string& shortCode) {
    const char* sql = "SELECT * FROM links WHERE short_code = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);
    
    model::ShortLink link;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        populateLinkFromRow(stmt, link);
        sqlite3_finalize(stmt);
        return link;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<model::ShortLink> Database::getLinkByCustomAlias(const std::string& customAlias) {
    const char* sql = "SELECT * FROM links WHERE custom_alias = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, customAlias.c_str(), -1, SQLITE_TRANSIENT);
    
    model::ShortLink link;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        populateLinkFromRow(stmt, link);
        sqlite3_finalize(stmt);
        return link;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<model::ShortLink> Database::getLinkByStatement(const char* sql, uint64_t id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    model::ShortLink link;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        populateLinkFromRow(stmt, link);
        sqlite3_finalize(stmt);
        return link;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

void Database::populateLinkFromRow(sqlite3_stmt* stmt, model::ShortLink& link) {
    link.id = sqlite3_column_int64(stmt, 0);
    link.longUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    link.shortCode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
        link.customAlias = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    link.createdAt = sqlite3_column_int64(stmt, 4);
    link.expireAt = sqlite3_column_int64(stmt, 5);
    link.status = static_cast<model::LinkStatus>(sqlite3_column_int(stmt, 6));
    link.visitCount = sqlite3_column_int(stmt, 7);
}

bool Database::updateLink(const model::ShortLink& link) {
    const char* sql = R"(
        UPDATE links 
        SET long_url = ?, short_code = ?, custom_alias = ?, 
            created_at = ?, expire_at = ?, status = ?, visit_count = ?
        WHERE id = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, link.longUrl.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, link.shortCode.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!link.customAlias.empty()) {
        sqlite3_bind_text(stmt, 3, link.customAlias.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    
    sqlite3_bind_int64(stmt, 4, link.createdAt);
    sqlite3_bind_int64(stmt, 5, link.expireAt);
    sqlite3_bind_int(stmt, 6, static_cast<int>(link.status));
    sqlite3_bind_int(stmt, 7, link.visitCount);
    sqlite3_bind_int64(stmt, 8, link.id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (success && sqlite3_changes(db_) > 0) {
        utils::LOG_INFO("Link updated: " + std::to_string(link.id));
    }
    
    return success && sqlite3_changes(db_) > 0;
}

bool Database::disableLink(uint64_t id) {
    const char* sql = "UPDATE links SET status = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, static_cast<int>(model::LinkStatus::DISABLED));
    sqlite3_bind_int64(stmt, 2, id);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (success && sqlite3_changes(db_) > 0) {
        utils::LOG_INFO("Link disabled: " + std::to_string(id));
    }
    
    return success && sqlite3_changes(db_) > 0;
}

bool Database::logVisit(const model::VisitLog& log) {
    // 开启事务
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    // 记录访问日志
    const char* insertVisitSql = R"(
        INSERT INTO visits (link_id, visit_time, ip_address, user_agent)
        VALUES (?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, insertVisitSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, log.linkId);
    sqlite3_bind_int64(stmt, 2, log.visitTime);
    sqlite3_bind_text(stmt, 3, log.ipAddress.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!log.userAgent.empty()) {
        sqlite3_bind_text(stmt, 4, log.userAgent.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (success) {
        // 增加访问计数
        incrementVisitCount(log.linkId);
        sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
        utils::LOG_INFO("Visit logged for link: " + std::to_string(log.linkId));
    } else {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
    }
    
    return success;
}

std::optional<model::LinkStats> Database::getLinkStats(uint64_t linkId, int limit) {
    // 获取链接信息
    auto linkOpt = getLinkById(linkId);
    if (!linkOpt.has_value()) {
        return std::nullopt;
    }
    
    model::LinkStats stats;
    stats.linkInfo = linkOpt.value();
    stats.totalVisits = stats.linkInfo.visitCount;
    
    // 获取最近的访问日志
    const char* sql = R"(
        SELECT visit_time, ip_address, user_agent 
        FROM visits 
        WHERE link_id = ? 
        ORDER BY visit_time DESC 
        LIMIT ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return stats;
    }
    
    sqlite3_bind_int64(stmt, 1, linkId);
    sqlite3_bind_int(stmt, 2, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        model::VisitLog log;
        log.linkId = linkId;
        log.visitTime = sqlite3_column_int64(stmt, 0);
        log.ipAddress = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            log.userAgent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        }
        
        stats.recentVisits.push_back(log);
    }
    
    sqlite3_finalize(stmt);
    return stats;
}

bool Database::isShortCodeExists(const std::string& shortCode) {
    return checkExists("SELECT 1 FROM links WHERE short_code = ?", shortCode);
}

bool Database::isCustomAliasExists(const std::string& customAlias) {
    return checkExists("SELECT 1 FROM links WHERE custom_alias = ?", customAlias);
}

bool Database::checkExists(const char* sql, const std::string& value) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_TRANSIENT);
    
    bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    
    return exists;
}

bool Database::incrementVisitCount(uint64_t linkId) {
    const char* sql = "UPDATE links SET visit_count = visit_count + 1 WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, linkId);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

} // namespace storage
