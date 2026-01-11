#pragma once

#include <string>
#include <optional>
#include "model/link.h"
#include "storage/database.h"
#include "storage/lru_cache.h"
#include "utils/shortcode_generator.h"
#include "utils/config.h"

namespace service {

class LinkService {
public:
    // 构造函数
    LinkService(std::shared_ptr<storage::Database> db, 
                std::shared_ptr<utils::ConfigManager> config);
    
    // 析构函数
    ~LinkService();
    
    // 初始化服务
    bool initialize();
    
    // 创建短链接
    model::CreateLinkResponse createShortLink(const model::CreateLinkRequest& request);
    
    // 根据短码获取短链接
    std::optional<model::ShortLink> getLinkByShortCode(const std::string& shortCode);
    
    // 根据ID获取短链接统计信息
    std::optional<model::LinkStats> getLinkStats(uint64_t linkId);
    
    // 禁用短链接
    bool disableLink(uint64_t linkId);
    
    // 记录访问日志
    bool logVisit(uint64_t linkId, const std::string& ipAddress, const std::string& userAgent = "");
    
    // 清理过期缓存
    void cleanupCache();
    
private:
    // 生成唯一的短码
    std::string generateUniqueShortCode();
    
    // 验证创建短链接的请求
    bool validateCreateRequest(const model::CreateLinkRequest& request, std::string& errorMessage);
    
    // 从缓存获取短链接
    std::optional<model::ShortLink> getLinkFromCache(const std::string& key);
    
    // 将短链接添加到缓存
    void addLinkToCache(const model::ShortLink& link);
    
    // 从缓存移除短链接
    void removeLinkFromCache(const std::string& key);
    
private:
    std::shared_ptr<storage::Database> database_;  // 数据库访问
    std::shared_ptr<utils::ConfigManager> config_;  // 配置管理器
    utils::ShortCodeGenerator shortCodeGenerator_;  // 短码生成器
    storage::LRUCache<std::string, model::ShortLink> linkCache_;  // 短链接缓存
    std::string baseUrl_;  // 基础URL
    int defaultExpireSeconds_;  // 默认过期时间（秒）
    int maxRetries_;  // 生成短码的最大重试次数
};

} // namespace service
