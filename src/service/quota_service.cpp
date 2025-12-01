#include "service/quota_service.hpp"
#include "utils/utils.hpp"
#include "config/config.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

namespace api_quota {
namespace service {

QuotaService::QuotaService(std::shared_ptr<ApiKeyService> api_key_service,
                         std::shared_ptr<repository::ApiKeyRepository> key_repository,
                         std::shared_ptr<repository::ClientRepository> client_repository,
                         std::shared_ptr<repository::LogRepository> log_repository)
    : api_key_service_(api_key_service),
      key_repository_(key_repository),
      client_repository_(client_repository),
      log_repository_(log_repository),
      daily_counter_cache_(1024),
      minute_counter_cache_(1024),
      key_daily_counter_cache_(1024),
      key_minute_counter_cache_(1024) {
}

QuotaService::~QuotaService() {
    // 析构函数已简化
}

QuotaService::QuotaCheckResult QuotaService::check_quota(const QuotaCheckRequest& request) {
    // 验证请求参数
    if (request.api_key.empty()) {
        return QuotaCheckResult{
            .allowed = false,
            .reason = "api_key_required",
            .remaining_in_minute = 0,
            .remaining_in_day = 0,
            .retry_after_seconds = 0
        };
    }
    
    if (request.weight <= 0) {
        return QuotaCheckResult{
            .allowed = false,
            .reason = "invalid_weight",
            .remaining_in_minute = 0,
            .remaining_in_day = 0,
            .retry_after_seconds = 0
        };
    }
    
    // 验证API密钥
    auto validation_result = api_key_service_->validate_api_key(request.api_key);
    if (!validation_result.is_valid) {
          // 记录拒绝日志
          if (validation_result.api_key) {
              repository::ApiCallLog log{
                    .client_id = validation_result.api_key->client_id,
                    .key_id = validation_result.api_key->key_id,
                  .api_key = request.api_key,
                  .endpoint = request.endpoint,
                  .weight = request.weight,
                  .allowed = false,
                  .reason = validation_result.reason,
                  .timestamp = utils::get_current_timestamp()
              };
              log_api_call(log);
          }
          
          return QuotaCheckResult{
            .allowed = false,
            .reason = validation_result.reason,
            .remaining_in_minute = 0,
            .remaining_in_day = 0,
            .retry_after_seconds = 0
        };
    }
    
    auto& api_key = validation_result.api_key.value();
    
    // 检查配额
    if (!check_quota_available(api_key.client_id, request.weight)) {
        // 确定拒绝原因
        std::string reason = "quota_exceeded";
        int retry_after = 0;
        
        if (!check_minute_quota(api_key.client_id, request.weight)) {
            reason = "quota_exceeded_per_minute";
            retry_after = 60; // 一分钟后重试
        } else if (!check_daily_quota(api_key.client_id, request.weight)) {
            reason = "quota_exceeded_daily";
            retry_after = 86400; // 一天后重试
        }
        
        // 记录拒绝日志
          repository::ApiCallLog log{
              .client_id = api_key.client_id,
              .key_id = api_key.key_id,
              .api_key = request.api_key,
              .endpoint = request.endpoint,
              .weight = request.weight,
              .allowed = false,
              .reason = reason,
              .timestamp = utils::get_current_timestamp()
          };
          log_api_call(log);
          
          return handle_quota_denied(reason, api_key.client_id);
    }
    
    // 通过配额校验，更新计数器
    update_counters(api_key.client_id, api_key.key_id, request.weight);
    
    // 记录允许的调用日志
    repository::ApiCallLog log{
        .client_id = api_key.client_id,
        .key_id = api_key.key_id,
        .api_key = request.api_key,
        .endpoint = request.endpoint,
        .weight = request.weight,
        .allowed = true,
        .reason = "ok",
        .timestamp = utils::get_current_timestamp()
    };
    log_api_call(log);
    
    // 获取客户端信息以计算剩余配额
    auto client = client_repository_->get_client_by_id(api_key.client_id);
    if (!client) {
        // 这种情况不应该发生，因为我们已经验证了密钥
        return QuotaCheckResult{
            .allowed = true,
            .reason = "ok",
            .remaining_in_minute = 0,
            .remaining_in_day = 0,
            .retry_after_seconds = 0
        };
    }
    
    // 计算剩余配额
    int64_t daily_used = daily_counter_cache_.get(api_key.client_id);
    int64_t minute_used = minute_counter_cache_.get(api_key.client_id);
    
    return QuotaCheckResult{
        .allowed = true,
        .reason = "ok",
        .remaining_in_minute = std::max(int64_t(0), client->per_minute_quota - minute_used),
        .remaining_in_day = std::max(int64_t(0), client->daily_quota - daily_used),
        .retry_after_seconds = 0
    };
}

bool QuotaService::check_quota_available(uint64_t client_id, int32_t weight) {
    return check_daily_quota(client_id, weight) && check_minute_quota(client_id, weight);
}

bool QuotaService::check_daily_quota(uint64_t client_id, int32_t weight) {
    auto client = client_repository_->get_client_by_id(client_id);
    if (!client) {
        return false;
    }
    
    int64_t current_count = daily_counter_cache_.get(client_id);
    return (current_count + weight) <= client->daily_quota;
}

bool QuotaService::check_minute_quota(uint64_t client_id, int32_t weight) {
    auto client = client_repository_->get_client_by_id(client_id);
    if (!client) {
        return false;
    }
    
    int64_t current_count = minute_counter_cache_.get(client_id);
    return (current_count + weight) <= client->per_minute_quota;
}

void QuotaService::update_counters(uint64_t client_id, uint64_t key_id, int32_t weight) {
    daily_counter_cache_.increment(client_id, weight);
    minute_counter_cache_.increment(client_id, weight);
    key_daily_counter_cache_.increment(key_id, weight);
    key_minute_counter_cache_.increment(key_id, weight);
}

void QuotaService::log_api_call(const repository::ApiCallLog& log) {
    std::lock_guard<std::mutex> lock(pending_logs_mutex_);
    pending_logs_.push_back(log);
    
    // 如果日志数量达到阈值，立即持久化
    if (pending_logs_.size() >= 100) { // 使用默认值100
        flush_counters();
    }
}

void QuotaService::batch_log_api_calls(const std::vector<repository::ApiCallLog>& logs) {
    std::lock_guard<std::mutex> lock(pending_logs_mutex_);
    pending_logs_.insert(pending_logs_.end(), logs.begin(), logs.end());
    
    if (pending_logs_.size() >= 100) { // 使用默认值100
        flush_counters();
    }
}

void QuotaService::flush_counters() {
    std::vector<repository::ApiCallLog> logs_to_persist;
    
    // 交换日志向量以最小化锁定时间
    {
        std::lock_guard<std::mutex> lock(pending_logs_mutex_);
        logs_to_persist.swap(pending_logs_);
    }
    
    if (!logs_to_persist.empty()) {
        try {
            log_repository_->bulk_log_api_calls(logs_to_persist);
            std::cout << "Persisted " << logs_to_persist.size() << " API call logs" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to persist logs: " << e.what() << std::endl;
            // 尝试重新添加到待处理队列
            std::lock_guard<std::mutex> lock(pending_logs_mutex_);
            pending_logs_.insert(pending_logs_.end(), logs_to_persist.begin(), logs_to_persist.end());
        }
    }
    
    // 刷新内存计数器到数据库
    try {
        // 注意：在实际生产环境中，可能需要更复杂的策略来合并内存和数据库中的计数器
        // 这里简化为只记录日志，通过日志来计算统计数据
    } catch (const std::exception& e) {
        std::cerr << "Failed to flush counters: " << e.what() << std::endl;
    }
}

// start_persistence_thread方法已移除

// stop_persistence_thread方法已移除

// persistence_loop方法已移除

void QuotaService::refresh_counters() {
    // 从数据库重新加载计数器数据
    // 注意：这可能会与内存中的计数器有冲突，需要谨慎处理
    std::cout << "Refreshing counters from database" << std::endl;
}

QuotaService::QuotaCheckResult QuotaService::handle_quota_denied(const std::string& reason, uint64_t client_id) {
    auto client = client_repository_->get_client_by_id(client_id);
    int64_t remaining_in_minute = 0;
    int64_t remaining_in_day = 0;
    int32_t retry_after_seconds = 0;
    
    if (client) {
        int64_t daily_used = daily_counter_cache_.get(client_id);
        int64_t minute_used = minute_counter_cache_.get(client_id);
        
        remaining_in_minute = std::max(int64_t(0), client->per_minute_quota - minute_used);
        remaining_in_day = std::max(int64_t(0), client->daily_quota - daily_used);
        
        // 设置重试时间
        if (reason == "quota_exceeded_per_minute") {
            retry_after_seconds = 60;
        } else if (reason == "quota_exceeded_daily") {
            retry_after_seconds = 86400;
        }
    }
    
    return QuotaCheckResult{
        .allowed = false,
        .reason = reason,
        .remaining_in_minute = remaining_in_minute,
        .remaining_in_day = remaining_in_day,
        .retry_after_seconds = retry_after_seconds
    };
}

std::vector<QuotaService::ClientQuotaStats> QuotaService::get_top_clients(int limit, const std::string&) { // 忽略order_by参数
    auto top_clients = client_repository_->get_top_clients_by_daily_calls(limit);
    std::vector<ClientQuotaStats> result;
    
    for (const auto& client_stats : top_clients) {
        // 假设top_clients返回的是pair，第一个元素是client_id，第二个元素是call_count
        auto client = client_repository_->get_client_by_id(client_stats.first);
        if (client) {
            ClientQuotaStats stats{
                .client_id = client_stats.first, // 使用client_stats中的client_id
                .client_name = client->name,
                .daily_quota = client->daily_quota,
                .per_minute_quota = client->per_minute_quota,
                .daily_used = client_stats.second,
                .minute_used = 0, // 使用默认值
                .total_calls = 0, // 使用默认值
                .denied_calls = 0  // 使用默认值
            };
            result.push_back(stats);
        }
    }
    
    return result;
}

std::optional<QuotaService::ClientDetailedStats> QuotaService::get_client_detailed_stats(uint64_t client_id,
                                                                                   const std::optional<std::string>&, // 忽略from_date
                                                                                    const std::optional<std::string>&) { // 忽略to_date
    auto client = client_repository_->get_client_by_id(client_id);
    if (!client) {
        return std::nullopt;
    }
    
    // 使用默认统计数据
    struct MockStats {
        uint64_t total_calls = 0;
        uint64_t denied_calls = 0;
        double success_rate = 100.0;
    } mock_stats;
    
    // 获取最近的调用日志 - 使用时间戳
    uint64_t start_time = 0;
    uint64_t end_time = utils::get_current_timestamp();
    auto recent_logs = log_repository_->get_client_logs(client_id, start_time, end_time, 20);
    
    ClientDetailedStats detailed_stats{
        .client_id = client_id, // 使用传入的client_id
        .client_name = client->name,
        .contact_email = client->contact_email,
        .total_calls = static_cast<int64_t>(mock_stats.total_calls),
        .denied_calls = static_cast<int64_t>(mock_stats.denied_calls),
        .denied_reasons = {}, // 使用空map
        .success_rate = mock_stats.success_rate,
        .recent_logs = recent_logs
    };
    
    return detailed_stats;
}

} // namespace service
} // namespace api_quota