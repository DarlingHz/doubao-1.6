#include "service/client_service.hpp"
#include "utils/utils.hpp"
#include "config/config.hpp"
#include <string>
#include <sstream>
#include <iostream>

namespace api_quota {
namespace service {

ClientService::ClientService(std::shared_ptr<repository::ClientRepository> repository)
    : repository_(repository),
      client_cache_(1000) { // 缓存最多1000个客户端对象
}

std::optional<repository::Client> ClientService::create_client(const std::string& name, 
                                                           const std::string& contact_email,
                                                           int64_t daily_quota, 
                                                           int64_t per_minute_quota) {
    // 验证参数
    if (name.empty()) {
        std::cerr << "Client name cannot be empty" << std::endl;
        return std::nullopt;
    }
    
    if (contact_email.empty()) {
        std::cerr << "Contact email cannot be empty" << std::endl;
        return std::nullopt;
    }
    
    if (daily_quota < 0 || per_minute_quota < 0) {
        std::cerr << "Quota values cannot be negative" << std::endl;
        return std::nullopt;
    }
    
    // 设置合理的默认值
    if (daily_quota == 0) {
        daily_quota = config::get_default_daily_quota();
    }
    
    if (per_minute_quota == 0) {
        per_minute_quota = config::get_default_per_minute_quota();
    }
    
    // 调用仓库层创建客户端
    auto client = repository_->create_client(name, contact_email, daily_quota, per_minute_quota);
    
    if (client) {
        std::cout << "Created new client with ID: " << client->client_id << std::endl;
        // 缓存新创建的客户端
        client_cache_.put(client->client_id, *client);
    }
    
    return client;
}

std::vector<repository::Client> ClientService::get_all_clients(bool include_inactive) {
    return repository_->get_all_clients(include_inactive);
}

std::optional<repository::Client> ClientService::get_client_by_id(uint64_t client_id) {
    // 先尝试从缓存获取
    auto cached_client = client_cache_.get(client_id);
    if (cached_client) {
        return *cached_client;
    }
    
    // 缓存未命中，从数据库获取
    auto client = repository_->get_client_by_id(client_id);
    
    // 更新缓存
    if (client) {
        client_cache_.put(client_id, *client);
    }
    
    return client;
}

bool ClientService::update_client(uint64_t client_id, const std::optional<std::string>& name,
                                const std::optional<std::string>& contact_email,
                                const std::optional<int64_t>& daily_quota,
                                const std::optional<int64_t>& per_minute_quota,
                                const std::optional<bool>& is_active) {
    bool success = repository_->update_client(client_id, name, contact_email, daily_quota, 
                                           per_minute_quota, is_active);
    
    if (success) {
        // 刷新缓存
        refresh_client_cache(client_id);
        std::cout << "Updated client with ID: " << client_id << std::endl;
    }
    
    return success;
}

bool ClientService::delete_client(uint64_t client_id) {
    // 逻辑删除客户端（设置为非活动状态）
    bool success = update_client(client_id, std::nullopt, std::nullopt, 
                               std::nullopt, std::nullopt, false);
    
    if (success) {
        std::cout << "Deleted client with ID: " << client_id << std::endl;
    }
    
    return success;
}

bool ClientService::is_client_active(uint64_t client_id) {
    auto client = get_client_by_id(client_id);
    return client.has_value() && client->is_active;
}

std::optional<ClientService::ClientQuotaStatus> ClientService::get_client_quota_status(uint64_t client_id) {
    auto client = get_client_by_id(client_id);
    if (!client) {
        return std::nullopt;
    }
    
    // 获取今日和当前分钟的使用情况
    // 暂时使用默认值，后续实现
    auto daily_used = 0; // 每日使用量
    auto minute_used = 0; // 每分钟使用量
    
    ClientQuotaStatus status{
        .client_id = client_id,
        .daily_quota = client->daily_quota,
        .per_minute_quota = client->per_minute_quota,
        .daily_used = daily_used,
        .minute_used = minute_used,
        .daily_remaining = std::max(int64_t(0), client->daily_quota - daily_used),
        .minute_remaining = std::max(int64_t(0), client->per_minute_quota - minute_used)
    };
    
    return status;
}

void ClientService::refresh_client_cache(uint64_t client_id) {
    // 删除旧缓存
    client_cache_.remove(client_id);
    
    // 重新加载客户端
    auto client = repository_->get_client_by_id(client_id);
    if (client) {
        client_cache_.put(client_id, *client);
    }
}

void ClientService::clear_cache() {
    client_cache_.clear();
    std::cout << "Client cache cleared" << std::endl;
}

std::string ClientService::generate_cache_key(uint64_t client_id) const {
    std::stringstream ss;
    ss << "client_" << client_id;
    return ss.str();
}

} // namespace service
} // namespace api_quota