#ifndef QUOTA_SERVICE_HPP
#define QUOTA_SERVICE_HPP

#include "repository/api_key_repository.hpp"
#include "repository/client_repository.hpp"
#include "repository/log_repository.hpp"
#include "service/api_key_service.hpp"
#include "utils/memory_cache.hpp"
#include "utils/memory_cache.hpp" // 使用memory_cache.hpp替代缺失的counter_cache.hpp
#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>

namespace api_quota {
namespace service {

class QuotaService {
public:
    explicit QuotaService(std::shared_ptr<ApiKeyService> api_key_service,
                        std::shared_ptr<repository::ApiKeyRepository> key_repository,
                        std::shared_ptr<repository::ClientRepository> client_repository,
                        std::shared_ptr<repository::LogRepository> log_repository);
    ~QuotaService();
    
    // 配额校验请求参数
    struct QuotaCheckRequest {
        std::string api_key;
        std::string endpoint;
        int32_t weight = 1; // 默认为1，可以根据接口复杂度设置不同权重
    };
    
    // 配额校验响应结果
    struct QuotaCheckResult {
        bool allowed;
        std::string reason;
        int64_t remaining_in_minute;
        int64_t remaining_in_day;
        int32_t retry_after_seconds = 0;
    };
    
    // 核心配额校验方法
    QuotaCheckResult check_quota(const QuotaCheckRequest& request);
    
    // 批量记录API调用
    void batch_log_api_calls(const std::vector<repository::ApiCallLog>& logs);
    
    // 刷新内存计数器
    void refresh_counters();
    
    // 强制将内存中的计数器数据持久化到数据库
    void flush_counters();
    
    // 启动后台持久化线程
    void start_persistence_thread();
    
    // 停止后台持久化线程
    void stop_persistence_thread();
    
    // 获取客户端配额统计
    struct ClientQuotaStats {
        uint64_t client_id;
        std::string client_name;
        int64_t daily_quota;
        int64_t per_minute_quota;
        int64_t daily_used;
        int64_t minute_used;
        int64_t total_calls;
        int64_t denied_calls;
    };
    
    // 获取排名靠前的客户端
    std::vector<ClientQuotaStats> get_top_clients(int limit, const std::string& order_by = "daily_calls");
    
    // 获取客户端详细统计
    struct ClientDetailedStats {
        uint64_t client_id;
        std::string client_name;
        std::string contact_email;
        int64_t total_calls;
        int64_t denied_calls;
        std::unordered_map<std::string, int64_t> denied_reasons;
        double success_rate;
        std::vector<repository::ApiCallLog> recent_logs;
    };
    
    std::optional<ClientDetailedStats> get_client_detailed_stats(uint64_t client_id, 
                                                             const std::optional<std::string>& from_date,
                                                             const std::optional<std::string>& to_date);
    
private:
    std::shared_ptr<ApiKeyService> api_key_service_;
    std::shared_ptr<repository::ApiKeyRepository> key_repository_;
    std::shared_ptr<repository::ClientRepository> client_repository_;
    std::shared_ptr<repository::LogRepository> log_repository_;
    
    // 内存中的计数器缓存
    utils::CounterCache<uint64_t> daily_counter_cache_;    // client_id -> daily count
    utils::CounterCache<uint64_t> minute_counter_cache_;   // client_id -> minute count
    utils::CounterCache<uint64_t> key_daily_counter_cache_; // key_id -> daily count
    utils::CounterCache<uint64_t> key_minute_counter_cache_; // key_id -> minute count
    
    // 批量日志缓存
    std::vector<repository::ApiCallLog> pending_logs_;
    std::mutex pending_logs_mutex_;
    
    // 移除后台线程相关代码
    
    // 检查配额是否足够
    bool check_quota_available(uint64_t client_id, int32_t weight);
    
    // 检查客户端是否超出每日配额
    bool check_daily_quota(uint64_t client_id, int32_t weight);
    
    // 检查客户端是否超出每分钟配额
    bool check_minute_quota(uint64_t client_id, int32_t weight);
    
    // 更新计数器
    void update_counters(uint64_t client_id, uint64_t key_id, int32_t weight);
    
    // 记录API调用日志
    void log_api_call(const repository::ApiCallLog& log);
    
    // 处理配额拒绝
    QuotaCheckResult handle_quota_denied(const std::string& reason, uint64_t client_id);
};

} // namespace service
} // namespace api_quota

#endif // QUOTA_SERVICE_HPP