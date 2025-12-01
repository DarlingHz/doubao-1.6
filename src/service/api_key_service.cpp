#include "service/api_key_service.hpp"
#include "utils/utils.hpp"
#include <string>
#include <iostream>
#include <chrono>

namespace api_quota {
namespace service {

ApiKeyService::ApiKeyService(std::shared_ptr<repository::ApiKeyRepository> key_repository,
                           std::shared_ptr<repository::ClientRepository> client_repository)
    : key_repository_(key_repository),
      client_repository_(client_repository),
      key_cache_(10000), // 缓存最多10000个API密钥
      id_cache_(10000) {
}

std::optional<repository::ApiKey> ApiKeyService::create_api_key(uint64_t client_id,
                                                            const std::optional<std::chrono::system_clock::time_point>& expire_time) {
    // 验证客户端是否存在且处于活动状态
    auto client = client_repository_->get_client_by_id(client_id);
    if (!client || !client->is_active) {
        std::cerr << "Invalid client ID or client is inactive: " << client_id << std::endl;
        return std::nullopt;
    }
    
    // 生成新的API密钥字符串
    std::string key_string = generate_api_key_string();
    
    // 调用仓库层创建API密钥
    // 确保expire_time参数类型正确
    uint64_t expire_time_uint64 = 0;
    if (expire_time) {
        // 转换time_point为timestamp
        auto duration = expire_time->time_since_epoch();
        expire_time_uint64 = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    auto api_key = key_repository_->create_api_key(client_id, key_string, "", expire_time_uint64);
    
    if (api_key) {
        std::cout << "Created new API key for client " << client_id << ": " << api_key->api_key << std::endl;
        // 缓存新创建的API密钥
        key_cache_.put(api_key->api_key, *api_key);
        id_cache_.put(api_key->key_id, *api_key);
    }
    
    return api_key;
}

std::vector<repository::ApiKey> ApiKeyService::get_client_api_keys(uint64_t client_id,
                                                                bool include_inactive) {
    // get_api_keys_by_client_id方法不支持include_inactive参数
    return key_repository_->get_api_keys_by_client_id(client_id);
}

std::optional<repository::ApiKey> ApiKeyService::get_api_key_by_key_string(const std::string& key_string) {
    // 先尝试从缓存获取
    auto cached_key = key_cache_.get(key_string);
    if (cached_key) {
        return *cached_key;
    }
    
    // 缓存未命中，从数据库获取
    auto api_key = key_repository_->get_api_key_by_key(key_string);
    if (api_key) {
        key_cache_.put(api_key->api_key, *api_key);
        id_cache_.put(api_key->key_id, *api_key);
    }
    
    return api_key;
}

std::optional<repository::ApiKey> ApiKeyService::get_api_key_by_id(uint64_t key_id) {
    // 先尝试从缓存获取
    auto cached_key = id_cache_.get(key_id);
    if (cached_key) {
        return *cached_key;
    }
    
    // 缓存未命中，从数据库获取
    auto api_key = key_repository_->get_api_key_by_id(key_id);
    
    // 更新缓存
    if (api_key) {
        key_cache_.put(api_key->api_key, *api_key);
        id_cache_.put(api_key->key_id, *api_key);
    }
    
    return api_key;
}

bool ApiKeyService::revoke_api_key(uint64_t key_id) {
    bool success = key_repository_->revoke_api_key(key_id);
    
    if (success) {
        // 刷新缓存
        auto api_key = get_api_key_by_id(key_id);
        if (api_key) {
            refresh_key_cache(api_key->api_key);
        }
        std::cout << "Revoked API key with ID: " << key_id << std::endl;
    }
    
    return success;
}

ApiKeyService::KeyValidationResult ApiKeyService::validate_api_key(const std::string& key_string) {
    KeyValidationResult result{
        .is_valid = false,
        .status = repository::ApiKeyStatus::REVOKED,
        .reason = "key_not_found",
        .api_key = std::nullopt
    };
    
    // 获取API密钥
    auto api_key = get_api_key_by_key_string(key_string);
    if (!api_key) {
        return result;
    }
    
    result.api_key = api_key;
    
    // 检查密钥状态
    repository::ApiKeyStatus key_status = api_key->get_status();
    if (key_status != repository::ApiKeyStatus::ACTIVE) {
        result.status = key_status;
        result.reason = "key_revoked";
        return result;
    }
    
    // 检查密钥是否过期
    if (is_key_expired(*api_key)) {
        result.status = repository::ApiKeyStatus::EXPIRED;
        result.reason = "key_expired";
        return result;
    }
    
    // 检查客户端是否处于活动状态
    auto client = client_repository_->get_client_by_id(api_key->client_id);
    if (!client || !client->is_active) {
        result.status = repository::ApiKeyStatus::REVOKED;
        result.reason = "client_inactive";
        return result;
    }
    
    // 验证通过
    result.is_valid = true;
    result.status = repository::ApiKeyStatus::ACTIVE;
    result.reason = "ok";
    return result;
}

std::optional<ApiKeyService::KeyUsageStats> ApiKeyService::get_key_usage_stats(uint64_t key_id) {
    auto api_key = get_api_key_by_id(key_id);
    if (!api_key) {
        return std::nullopt;
    }
    
    // 获取密钥的使用统计
    auto stats = key_repository_->get_key_usage_stats(key_id);
    if (!stats) {
        return std::nullopt;
    }
    
    // 获取客户端配额信息
    auto client = client_repository_->get_client_by_id(api_key->client_id);
    if (!client) {
        return std::nullopt;
    }
    
    // 计算今日剩余配额 - 已经在上面定义过stats，直接使用
    if (!stats) {
        // 如果获取统计失败，返回默认值
        KeyUsageStats usage_stats{
            .key_id = key_id,
            .key_string = api_key->api_key,
            .total_calls = 0,
            .today_calls = 0,
            .remaining_quota_today = client->daily_quota,
            .remaining_quota_minute = client->per_minute_quota
        };
        return usage_stats;
    }
    auto today_calls = stats->today_calls;
    auto minute_calls = stats->current_minute_calls;
    
    KeyUsageStats usage_stats{
        .key_id = key_id,
        .key_string = api_key->api_key,
        .total_calls = stats->total_calls,
        .today_calls = stats->today_calls,
        // KeyUsageStats中没有failed_calls和success_rate字段
        .remaining_quota_today = std::max(int64_t(0), int64_t(client->daily_quota - today_calls)),
        .remaining_quota_minute = std::max(int64_t(0), int64_t(client->per_minute_quota - minute_calls))
    };
    
    return usage_stats;
}

std::vector<std::pair<uint64_t, int64_t>> ApiKeyService::get_key_timeline(uint64_t key_id, int hours_back) {
    return key_repository_->get_key_timeline(key_id, hours_back);
}

void ApiKeyService::refresh_key_cache(const std::string& key_string) {
    // 删除旧缓存
    key_cache_.remove(key_string);
    
    // 重新加载API密钥
    auto api_key = key_repository_->get_api_key_by_key(key_string);
    if (api_key) {
        key_cache_.put(api_key->api_key, *api_key);
        id_cache_.put(api_key->key_id, *api_key);
    }
}

void ApiKeyService::clear_cache() {
    key_cache_.clear();
    id_cache_.clear();
    std::cout << "API key cache cleared" << std::endl;
}

std::string ApiKeyService::generate_api_key_string() const {
    // 生成32位的随机字符串作为API密钥
    return utils::generate_random_string(32);
}

bool ApiKeyService::is_key_expired(const repository::ApiKey& key) const {
    // 使用正确的expires_at成员和处理方式
    uint64_t now = utils::get_current_timestamp();
    return key.expires_at > 0 && now > key.expires_at;
}

} // namespace service
} // namespace api_quota