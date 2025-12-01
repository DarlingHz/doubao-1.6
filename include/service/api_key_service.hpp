#ifndef API_KEY_SERVICE_HPP
#define API_KEY_SERVICE_HPP

#include "repository/api_key_repository.hpp"
#include "repository/client_repository.hpp"
#include "utils/memory_cache.hpp"
#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <memory>
#include <chrono>

namespace api_quota {
namespace service {

class ApiKeyService {
public:
    explicit ApiKeyService(std::shared_ptr<repository::ApiKeyRepository> key_repository,
                         std::shared_ptr<repository::ClientRepository> client_repository);
    ~ApiKeyService() = default;
    
    // 为客户端创建新的API密钥
    std::optional<repository::ApiKey> create_api_key(uint64_t client_id,
                                                  const std::optional<std::chrono::system_clock::time_point>& expire_time = std::nullopt);
    
    // 获取客户端的所有API密钥
    std::vector<repository::ApiKey> get_client_api_keys(uint64_t client_id,
                                                     bool include_inactive = true);
    
    // 根据API密钥字符串获取API密钥信息
    std::optional<repository::ApiKey> get_api_key_by_key_string(const std::string& key_string);
    
    // 根据ID获取API密钥
    std::optional<repository::ApiKey> get_api_key_by_id(uint64_t key_id);
    
    // 吊销API密钥
    bool revoke_api_key(uint64_t key_id);
    
    // 验证API密钥是否有效
    struct KeyValidationResult {
        bool is_valid;
        repository::ApiKeyStatus status;
        std::string reason;
        std::optional<repository::ApiKey> api_key;
    };
    
    KeyValidationResult validate_api_key(const std::string& key_string);
    
    // 获取API密钥的使用统计
    struct KeyUsageStats {
        uint64_t key_id;
        std::string key_string;
        uint64_t total_calls;
        uint64_t today_calls;
        uint64_t failed_calls;
        double success_rate;
        int64_t remaining_quota_today;
        int64_t remaining_quota_minute;
    };
    
    std::optional<KeyUsageStats> get_key_usage_stats(uint64_t key_id);
    
    // 获取API密钥的时间线数据
    // 修正时间线返回类型，与ApiKeyRepository::get_key_timeline匹配
    std::vector<std::pair<uint64_t, int64_t>> get_key_timeline(uint64_t key_id, int hours_back = 24);
    
    // 刷新API密钥缓存
    void refresh_key_cache(const std::string& key_string);
    
    // 清除所有缓存
    void clear_cache();
    
private:
    std::shared_ptr<repository::ApiKeyRepository> key_repository_;
    std::shared_ptr<repository::ClientRepository> client_repository_;
    utils::MemoryCache<std::string, repository::ApiKey> key_cache_; // 按密钥字符串缓存
    utils::MemoryCache<uint64_t, repository::ApiKey> id_cache_; // 按ID缓存
    
    // 生成新的API密钥字符串
    std::string generate_api_key_string() const;
    
    // 检查密钥是否过期
    bool is_key_expired(const repository::ApiKey& key) const;
};

} // namespace service
} // namespace api_quota

#endif // API_KEY_SERVICE_HPP