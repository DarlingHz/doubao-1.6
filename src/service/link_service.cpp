#include "../include/service/link_service.h"
#include "../include/utils/logger.h"
#include "../include/utils/url_utils.h"
#include "../include/utils/time_utils.h"
#include "../include/storage/database.h"
#include "../include/utils/shortcode_generator.h"
#include <memory>

namespace service {

LinkService::LinkService(std::shared_ptr<storage::Database> db, 
                         std::shared_ptr<utils::ConfigManager> config)
    : database_(db), 
      config_(config),
      shortCodeGenerator_(config->getShortCodeLength()),
      linkCache_(config->getCacheCapacity(), config->getCacheTTLSeconds()),
      defaultExpireSeconds_(config->getDefaultExpireSeconds()),
      maxRetries_(10) {
    // 构建基础URL
    baseUrl_ = "http://" + config->getHttpHost() + ":" + std::to_string(config->getHttpPort());
}

LinkService::~LinkService() {
    utils::LOG_INFO("LinkService destroyed");
}

bool LinkService::initialize() {
    if (!database_) {
        utils::LOG_ERROR("Database not initialized");
        return false;
    }

    utils::LOG_INFO("LinkService initialized with cache capacity: " + std::to_string(config_->getCacheCapacity()));
    return true;
}

model::CreateLinkResponse LinkService::createShortLink(const model::CreateLinkRequest& request) {
    model::CreateLinkResponse response;
    std::string errorMessage;

    // 验证请求
    if (!validateCreateRequest(request, errorMessage)) {
        response.success = false;
        response.message = errorMessage;
        return response;
    }

    // 处理自定义别名
    std::string shortCode;
    if (!request.customAlias.empty()) {
        // 检查自定义别名是否已存在
        if (database_->isCustomAliasExists(request.customAlias)) {
            response.success = false;
            response.message = "Custom alias already exists";
            return response;
        }
        shortCode = request.customAlias;
    } else {
        // 生成唯一的短码
        shortCode = generateUniqueShortCode();
        if (shortCode.empty()) {
            response.success = false;
            response.message = "Failed to generate unique short code";
            return response;
        }
    }

    // 创建短链接对象
    model::ShortLink link;
    link.longUrl = request.longUrl;
    link.shortCode = shortCode;
    link.customAlias = request.customAlias;
    link.createdAt = utils::TimeUtils::getCurrentTimestamp();
    link.expireAt = utils::TimeUtils::calculateExpireTime(
        link.createdAt, request.expireSeconds > 0 ? request.expireSeconds : defaultExpireSeconds_);
    link.status = model::LinkStatus::ACTIVE;
    link.visitCount = 0;

    // 保存到数据库
    auto linkIdOpt = database_->createLink(link);
    if (!linkIdOpt) {
        response.success = false;
        response.message = "Failed to create short link";
        return response;
    }

    link.id = *linkIdOpt;

    // 添加到缓存
    addLinkToCache(link);

    // 构建响应
    response.success = true;
    response.message = "Short link created successfully";
    response.shortCode = shortCode;
    response.fullShortUrl = utils::UrlUtils::buildShortUrl(baseUrl_, shortCode);
    response.expireAt = link.expireAt;

    utils::LOG_INFO("Created short link: " + shortCode + " -> " + request.longUrl);
    return response;
}

std::optional<model::ShortLink> LinkService::getLinkByShortCode(const std::string& shortCode) {
    // 先从缓存获取
    auto cachedLink = getLinkFromCache(shortCode);
    if (cachedLink) {
        // 检查是否过期
        if (cachedLink->isAccessible()) {
            return cachedLink;
        }
        // 如果已过期，从缓存中移除
        removeLinkFromCache(shortCode);
    }

    // 从数据库查询
    auto linkOpt = database_->getLinkByShortCode(shortCode);
    if (!linkOpt) {
        // 尝试通过自定义别名查询
        linkOpt = database_->getLinkByCustomAlias(shortCode);
        if (!linkOpt) {
            return std::nullopt;
        }
    }

    // 检查是否可访问
    if (!linkOpt->isAccessible()) {
        return std::nullopt;
    }

    // 添加到缓存
    addLinkToCache(*linkOpt);

    return linkOpt;
}

std::optional<model::LinkStats> LinkService::getLinkStats(uint64_t linkId) {
    // 直接从数据库获取统计信息
    auto statsOpt = database_->getLinkStats(linkId);
    if (!statsOpt) {
        utils::LOG_ERROR("Failed to get link stats for id: " + std::to_string(linkId));
        return std::nullopt;
    }

    return statsOpt;
}

bool LinkService::disableLink(uint64_t linkId) {
    // 先检查短链接是否存在
    auto linkOpt = database_->getLinkById(linkId);
    if (!linkOpt) {
        utils::LOG_ERROR("Link not found with id: " + std::to_string(linkId));
        return false;
    }

    // 禁用短链接
    if (!database_->disableLink(linkId)) {
        utils::LOG_ERROR("Failed to disable link with id: " + std::to_string(linkId));
        return false;
    }

    // 从缓存中移除
    removeLinkFromCache(linkOpt->shortCode);
    if (!linkOpt->customAlias.empty()) {
        removeLinkFromCache(linkOpt->customAlias);
    }

    utils::LOG_INFO("Disabled link with id: " + std::to_string(linkId));
    return true;
}

bool LinkService::logVisit(uint64_t linkId, const std::string& ipAddress, const std::string& userAgent) {
    model::VisitLog log;
    log.linkId = linkId;
    log.visitTime = utils::TimeUtils::getCurrentTimestamp();
    log.ipAddress = ipAddress;
    log.userAgent = userAgent;

    bool success = database_->logVisit(log);
    if (success) {
        utils::LOG_INFO("Logged visit for link id: " + std::to_string(linkId) + " from IP: " + ipAddress);
    } else {
        utils::LOG_ERROR("Failed to log visit for link id: " + std::to_string(linkId));
    }

    return success;
}

void LinkService::cleanupCache() {
    linkCache_.cleanupExpired();
    utils::LOG_INFO("Cache cleanup completed, current size: " + std::to_string(linkCache_.size()));
}

std::string LinkService::generateUniqueShortCode() {
    for (int i = 0; i < maxRetries_; ++i) {
        std::string shortCode = shortCodeGenerator_.generateRandomCode();
        
        // 检查短码是否已存在
        if (!database_->isShortCodeExists(shortCode)) {
            return shortCode;
        }
    }

    utils::LOG_ERROR("Failed to generate unique short code after " + std::to_string(maxRetries_) + " attempts");
    return "";
}

bool LinkService::validateCreateRequest(const model::CreateLinkRequest& request, std::string& errorMessage) {
    // 验证长链接
    if (request.longUrl.empty()) {
        errorMessage = "Long URL is required";
        return false;
    }

    if (!utils::UrlUtils::isValidUrl(request.longUrl)) {
        errorMessage = "Invalid URL format";
        return false;
    }

    // 验证自定义别名
    if (!request.customAlias.empty()) {
        if (!utils::UrlUtils::isValidCustomAlias(request.customAlias, config_->getMaxCustomAliasLength())) {
            errorMessage = "Invalid custom alias format";
            return false;
        }

        // 检查自定义别名是否与保留格式冲突（如以/s/开头）
        if (request.customAlias.substr(0, 2) == "s/") {
            errorMessage = "Custom alias cannot start with 's/'";
            return false;
        }
    }

    // 验证过期时间
    if (request.expireSeconds < 0) {
        errorMessage = "Expire seconds cannot be negative";
        return false;
    }

    return true;
}

std::optional<model::ShortLink> LinkService::getLinkFromCache(const std::string& key) {
    return linkCache_.get(key);
}

void LinkService::addLinkToCache(const model::ShortLink& link) {
    // 计算缓存的TTL
    uint64_t ttl = 0;
    if (link.expireAt > 0) {
        uint64_t now = utils::TimeUtils::getCurrentTimestamp();
        if (link.expireAt > now) {
            ttl = link.expireAt - now;
        } else {
            return;  // 已过期，不缓存
        }
    }

    // 同时缓存短码和自定义别名（如果有）
    linkCache_.set(link.shortCode, link, ttl);
    if (!link.customAlias.empty()) {
        linkCache_.set(link.customAlias, link, ttl);
    }
}

void LinkService::removeLinkFromCache(const std::string& key) {
    linkCache_.remove(key);
}

} // namespace service
