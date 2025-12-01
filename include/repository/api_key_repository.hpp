#ifndef API_KEY_REPOSITORY_HPP
#define API_KEY_REPOSITORY_HPP

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <memory>

namespace api_quota {
namespace repository {

enum class ApiKeyStatus {
    ACTIVE,
    REVOKED,
    EXPIRED
};

struct ApiKey {
    uint64_t key_id = 0;
    uint64_t client_id = 0;
    std::string api_key;
    std::string description;
    uint64_t expires_at = 0;
    bool is_revoked = false;
    uint64_t created_at = 0;
    uint64_t updated_at = 0;
    
    // 获取状态
    ApiKeyStatus get_status() const;
};

class ApiKeyRepository {
public:
    ApiKeyRepository(const std::string& database_path);
    ~ApiKeyRepository();
    
    // 初始化数据库表
    bool init_database();
    
    // 创建新的API Key
    std::optional<ApiKey> create_api_key(uint64_t client_id, const std::string& api_key,
                                       const std::string& description = "",
                                       uint64_t expires_at = 0);
    
    // 根据客户端ID获取所有API Key
    std::vector<ApiKey> get_api_keys_by_client_id(uint64_t client_id);
    
    // 根据API Key字符串查找
    std::optional<ApiKey> get_api_key_by_key(const std::string& api_key);
    
    // 根据ID获取API Key
    std::optional<ApiKey> get_api_key_by_id(uint64_t key_id);
    
    // 吊销API Key
    bool revoke_api_key(uint64_t key_id);
    
    // 批量吊销客户端的所有API Key
    bool revoke_all_client_keys(uint64_t client_id);
    
    // 验证API Key是否有效
    bool is_key_valid(const std::string& api_key);
    
    // 获取客户端的API Key使用统计
    struct KeyUsageStats {
        uint64_t total_calls = 0;
        uint64_t today_calls = 0;
        uint64_t current_minute_calls = 0;
        uint64_t last_used_at = 0;
    };
    
    std::optional<KeyUsageStats> get_key_usage_stats(uint64_t key_id);
    
    // 获取API Key的时间线数据（按小时聚合）
    std::vector<std::pair<uint64_t, int64_t>> get_key_timeline(uint64_t key_id, 
                                                             int hours_back = 24);
    
private:
    std::string database_path_;
    
    // SQLite相关操作（内部实现）
    class SQLiteImpl;
    std::unique_ptr<SQLiteImpl> impl_;
};

} // namespace repository
} // namespace api_quota

#endif // API_KEY_REPOSITORY_HPP