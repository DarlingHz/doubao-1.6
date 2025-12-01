#ifndef STATS_SERVICE_HPP
#define STATS_SERVICE_HPP

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include "repository/log_repository.hpp"
#include "repository/client_repository.hpp"
#include "utils/memory_cache.hpp"

namespace api_quota {
namespace service {

// 客户端统计数据结构
struct ClientStats {
    uint64_t client_id;
    std::string client_name;
    std::string contact_email;
    uint64_t total_calls;
    uint64_t denied_calls;
    double success_rate;
    std::unordered_map<std::string, uint64_t> denied_reasons;
    std::vector<repository::ApiCallLog> recent_logs;
};

// 时间段统计数据结构
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    uint64_t calls;
    uint64_t allowed;
    uint64_t denied;
};

// 端点使用统计
struct EndpointStats {
    std::string endpoint;
    uint64_t total_calls;
    uint64_t allowed_calls;
    uint64_t denied_calls;
    double avg_response_time;
};

class StatsService {
public:
    StatsService(std::shared_ptr<repository::LogRepository> log_repository,
                std::shared_ptr<repository::ClientRepository> client_repository,
                std::shared_ptr<void> cache);
    
    ~StatsService() = default;
    
    // 获取客户端详细统计信息
    std::optional<ClientStats> get_client_stats(uint64_t client_id,
                                              std::optional<std::string> from_date = std::nullopt,
                                              std::optional<std::string> to_date = std::nullopt);
    
    // 获取客户端使用统计
    std::optional<std::vector<TimeSeriesPoint>> get_client_timeline(uint64_t client_id,
                                                                   int granularity = 1);
    
    // 获取热门客户端（按调用量排序）
    std::vector<ClientStats> get_top_clients(size_t limit = 10,
                                           const std::string& order_by = "daily_calls");
    
    // 获取热门端点（按调用量排序）
    std::vector<EndpointStats> get_top_endpoints(size_t limit = 10,
                                               std::optional<std::string> from_date = std::nullopt,
                                               std::optional<std::string> to_date = std::nullopt);
    
    // 获取系统总体统计
    std::unordered_map<std::string, uint64_t> get_system_stats();
    
    // 获取最近的API调用日志
    std::vector<repository::ApiCallLog> get_recent_logs(size_t limit = 100);
    
    // 获取拒绝原因分布
    std::unordered_map<std::string, uint64_t> get_denial_reason_distribution(
        std::optional<uint64_t> client_id = std::nullopt,
        std::optional<std::string> from_date = std::nullopt,
        std::optional<std::string> to_date = std::nullopt);
    
    // 生成每日报告
    std::string generate_daily_report();
    
private:
    // 成员变量
    std::shared_ptr<repository::LogRepository> log_repository_;
    std::shared_ptr<repository::ClientRepository> client_repository_;
    std::shared_ptr<void> cache_; // 简化实现，避免使用模板
    
    // 缓存键生成函数
    std::string generate_cache_key(const std::string& prefix, const std::vector<std::string>& components);
};

} // namespace service
} // namespace api_quota

#endif // STATS_SERVICE_HPP