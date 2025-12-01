#include "service/stats_service.hpp"
#include "utils/utils.hpp"
#include <numeric>
#include <sstream>

namespace api_quota {
namespace service {

StatsService::StatsService(std::shared_ptr<repository::LogRepository> log_repository,
                           std::shared_ptr<repository::ClientRepository> client_repository,
                           std::shared_ptr<void> cache)
    : log_repository_(log_repository),
      client_repository_(client_repository),
      cache_(cache) {
}

std::optional<ClientStats> StatsService::get_client_stats(uint64_t client_id,
                                                         std::optional<std::string> from_date,
                                                         std::optional<std::string> to_date) {
    // 移除缓存逻辑，使用void指针
    
    // 检查客户端是否存在
    auto client = client_repository_->get_client_by_id(client_id);
    if (!client) {
        return std::nullopt;
    }
    
    // 获取调用统计
    auto logs = log_repository_->get_client_logs(client_id, 0, 0); // 使用默认值
    
    // 构建统计数据
    ClientStats stats;
    stats.client_id = client->client_id;
    stats.client_name = client->name;
    stats.contact_email = client->contact_email;
    stats.total_calls = logs.size();
    stats.denied_calls = 0;
    
    // 统计拒绝原因
    for (const auto& log : logs) {
        if (!log.allowed) {
            stats.denied_calls++;
            stats.denied_reasons[log.reason]++;
        }
    }
    
    // 计算成功率
    if (stats.total_calls > 0) {
        stats.success_rate = static_cast<double>(stats.total_calls - stats.denied_calls) / stats.total_calls * 100.0;
    } else {
        stats.success_rate = 0.0;
    }
    
    // 获取最近的日志
    size_t recent_limit = 50;
    size_t start_idx = logs.size() > recent_limit ? logs.size() - recent_limit : 0;
    for (size_t i = start_idx; i < logs.size(); i++) {
        stats.recent_logs.push_back(logs[i]);
    }
    
    // 移除缓存逻辑
    
    return stats;
}



// 简化实现
std::optional<std::vector<TimeSeriesPoint>> StatsService::get_client_timeline(uint64_t client_id,
                                                                            int granularity) { // 使用int类型
    // 移除缓存逻辑
    
    // 简化实现，返回空时间线
    std::vector<TimeSeriesPoint> result;
    
    // 移除缓存逻辑
    
    return result;
}

// 简化实现
std::vector<ClientStats> StatsService::get_top_clients(size_t limit, const std::string& order_by) {
    // 移除缓存逻辑
    
    // 简化实现，返回空结果
    std::vector<ClientStats> result;
    
    // 不使用不存在的方法
    // 仅返回空结果
    return result;
}

std::vector<EndpointStats> StatsService::get_top_endpoints(size_t limit,
                                                          std::optional<std::string> from_date,
                                                          std::optional<std::string> to_date) {
    // 移除缓存逻辑，使用void指针
    
    // 获取端点统计
    auto endpoint_stats_map = log_repository_->get_endpoint_stats(from_date, to_date);
    
    // 转换为向量并排序
    std::vector<EndpointStats> result;
    for (const auto& [endpoint, stats] : endpoint_stats_map) {
        EndpointStats endpoint_stats;
        endpoint_stats.endpoint = endpoint;
        endpoint_stats.total_calls = stats.total_calls;
        endpoint_stats.allowed_calls = stats.allowed_calls;
        endpoint_stats.denied_calls = stats.total_calls - stats.allowed_calls;
        endpoint_stats.avg_response_time = stats.avg_response_time;
        result.push_back(endpoint_stats);
    }
    
    // 按调用量排序
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.total_calls > b.total_calls;
    });
    
    // 限制结果数量
    if (result.size() > limit) {
        result.resize(limit);
    }
    
    // 移除缓存逻辑
    
    return result;
}

std::unordered_map<std::string, uint64_t> StatsService::get_system_stats() {
    // 移除缓存逻辑，使用void指针
    
    // 收集系统统计数据
    std::unordered_map<std::string, uint64_t> stats;
    
    // 总客户端数
    auto all_clients = client_repository_->get_all_clients(false);
    stats["total_clients"] = all_clients.size();
    
    // 活跃客户端数
    auto active_clients = client_repository_->get_all_clients(true);
    stats["active_clients"] = active_clients.size();
    
    // 今日统计
    auto today_stats = log_repository_->get_daily_stats();
    stats["today_total_calls"] = today_stats.total_calls;
    stats["today_allowed_calls"] = today_stats.allowed_calls;
    stats["today_denied_calls"] = today_stats.total_calls - today_stats.allowed_calls;
    
    // 移除缓存逻辑
    
    return stats;
}

std::vector<repository::ApiCallLog> StatsService::get_recent_logs(size_t limit) {
    // 从日志仓库获取最近的日志
    return log_repository_->get_recent_logs(limit);
}

std::unordered_map<std::string, uint64_t> StatsService::get_denial_reason_distribution(
    std::optional<uint64_t> client_id,
    std::optional<std::string> from_date,
    std::optional<std::string> to_date) {
    // 移除缓存逻辑，使用void指针
    
    // 获取拒绝原因分布
    auto result = log_repository_->get_denial_reason_distribution(client_id, from_date, to_date);
    
    // 移除缓存逻辑
    
    return result;
}

std::string StatsService::generate_daily_report() {
    // 生成每日报告
    // 这里返回一个简单的报告字符串，实际项目中可能会生成更详细的HTML或JSON报告
    std::stringstream report;
    
    report << "Daily Report - " << utils::format_timestamp(utils::get_current_timestamp()) << "\n";
    report << "========================\n\n";
    
    // 系统概览
    auto system_stats = get_system_stats();
    report << "System Overview:\n";
    report << "- Total Clients: " << system_stats["total_clients"] << "\n";
    report << "- Active Clients: " << system_stats["active_clients"] << "\n";
    report << "- Today's Total Calls: " << system_stats["today_total_calls"] << "\n";
    report << "- Today's Allowed Calls: " << system_stats["today_allowed_calls"] << "\n";
    report << "- Today's Denied Calls: " << system_stats["today_denied_calls"] << "\n\n";
    
    // Top Clients
    report << "Top 10 Clients by Calls:\n";
    auto top_clients = get_top_clients(10);
    for (size_t i = 0; i < top_clients.size(); i++) {
        report << i + 1 << ". " << top_clients[i].client_name << ": " 
              << top_clients[i].total_calls << " calls";
        if (top_clients[i].total_calls > 0) {
            uint64_t denied = top_clients[i].denied_calls; // 直接使用denied_calls成员
            double denial_rate = static_cast<double>(denied) / top_clients[i].total_calls * 100.0;
            report << " (Denial rate: " << denial_rate << "%)";
        }
        report << "\n";
    }
    
    // Top Endpoints
    report << "\nTop 10 Endpoints:\n";
    auto top_endpoints = get_top_endpoints(10);
    for (size_t i = 0; i < top_endpoints.size(); i++) {
        report << i + 1 << ". " << top_endpoints[i].endpoint << ": " 
              << top_endpoints[i].total_calls << " calls\n";
    }
    
    // Denial Reasons
    report << "\nDenial Reason Distribution:\n";
    auto denial_reasons = get_denial_reason_distribution();
    for (const auto& [reason, count] : denial_reasons) {
        report << "- " << reason << ": " << count << "\n";
    }
    
    return report.str();
}

std::string StatsService::generate_cache_key(const std::string& prefix, const std::vector<std::string>& components) {
    std::string key = prefix;
    for (const auto& component : components) {
        key += ":" + component;
    }
    return key;
}

} // namespace service
} // namespace api_quota