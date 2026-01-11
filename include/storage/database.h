#pragma once

#include "model/link.h"
#include "utils/logger.h"
#include <sqlite3.h>
#include <string>
#include <optional>
#include <vector>

namespace storage {

class Database {
public:
    // 构造函数
    explicit Database(const std::string& dbPath);
    
    // 析构函数
    ~Database();
    
    // 初始化数据库（创建表等）
    bool initialize();
    
    // 创建短链接
    std::optional<uint64_t> createLink(const model::ShortLink& link);
    
    // 根据ID获取短链接
    std::optional<model::ShortLink> getLinkById(uint64_t id);
    
    // 根据短码获取短链接
    std::optional<model::ShortLink> getLinkByShortCode(const std::string& shortCode);
    
    // 根据自定义别名获取短链接
    std::optional<model::ShortLink> getLinkByCustomAlias(const std::string& customAlias);
    
    // 根据SQL语句获取短链接（内部使用）
    std::optional<model::ShortLink> getLinkByStatement(const char* sql, uint64_t id);
    
    // 更新短链接
    bool updateLink(const model::ShortLink& link);
    
    // 禁用短链接
    bool disableLink(uint64_t id);
    
    // 记录访问日志
    bool logVisit(const model::VisitLog& log);
    
    // 获取短链接统计信息
    std::optional<model::LinkStats> getLinkStats(uint64_t linkId, int limit = 100);
    
    // 检查短码是否已存在
    bool isShortCodeExists(const std::string& shortCode);
    
    // 检查自定义别名是否已存在
    bool isCustomAliasExists(const std::string& customAlias);
    
    // 增加访问计数
    bool incrementVisitCount(uint64_t linkId);
    
private:
    // 数据库连接
    sqlite3* db_;
    
    // 数据库路径
    std::string dbPath_;
    
    // 创建表
    bool createTables();
    
    // 辅助方法
    bool executeStatement(const char* sql);
    bool checkExists(const char* sql, const std::string& value);
    void populateLinkFromRow(sqlite3_stmt* stmt, model::ShortLink& link);
};

} // namespace storage
