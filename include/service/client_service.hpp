#ifndef CLIENT_SERVICE_HPP
#define CLIENT_SERVICE_HPP

#include "repository/client_repository.hpp"
#include "utils/memory_cache.hpp"
#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <memory>

namespace api_quota {
namespace service {

class ClientService {
public:
    explicit ClientService(std::shared_ptr<repository::ClientRepository> repository);
    ~ClientService() = default;
    
    // 创建新客户端
    std::optional<repository::Client> create_client(const std::string& name, 
                                                 const std::string& contact_email,
                                                 int64_t daily_quota, 
                                                 int64_t per_minute_quota);
    
    // 获取所有客户端
    std::vector<repository::Client> get_all_clients(bool include_inactive = false);
    
    // 根据ID获取客户端
    std::optional<repository::Client> get_client_by_id(uint64_t client_id);
    
    // 更新客户端信息
    bool update_client(uint64_t client_id, const std::optional<std::string>& name,
                      const std::optional<std::string>& contact_email,
                      const std::optional<int64_t>& daily_quota,
                      const std::optional<int64_t>& per_minute_quota,
                      const std::optional<bool>& is_active);
    
    // 删除客户端（逻辑删除）
    bool delete_client(uint64_t client_id);
    
    // 验证客户端是否存在且处于活动状态
    bool is_client_active(uint64_t client_id);
    
    // 获取客户端的当前配额状态
    struct ClientQuotaStatus {
        uint64_t client_id;
        int64_t daily_quota;
        int64_t per_minute_quota;
        int64_t daily_used;
        int64_t minute_used;
        int64_t daily_remaining;
        int64_t minute_remaining;
    };
    
    std::optional<ClientQuotaStatus> get_client_quota_status(uint64_t client_id);
    
    // 刷新客户端缓存
    void refresh_client_cache(uint64_t client_id);
    
    // 清除所有缓存
    void clear_cache();
    
private:
    std::shared_ptr<repository::ClientRepository> repository_;
    utils::MemoryCache<uint64_t, repository::Client> client_cache_;
    
    // 缓存键生成
    std::string generate_cache_key(uint64_t client_id) const;
};

} // namespace service
} // namespace api_quota

#endif // CLIENT_SERVICE_HPP