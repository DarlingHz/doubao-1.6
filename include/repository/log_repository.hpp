#ifndef LOG_REPOSITORY_HPP
#define LOG_REPOSITORY_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <optional>

namespace api_quota {
namespace repository {

struct ApiCallLog {
    uint64_t log_id = 0;
    uint64_t client_id = 0;
    uint64_t key_id = 0;
    std::string api_key;
    std::string endpoint;
    int64_t weight = 1;
    bool allowed = false;
    std::string reason;
    uint64_t retry_after = 0;
    uint64_t timestamp = 0;
    std::string ip_address;
};

class LogRepository {
public:
    LogRepository(const std::string& database_path);
    ~LogRepository();
    
    // 初始化数据库表
    bool init_database();
    
    // 记录API调用日志
    bool log_api_call(uint64_t client_id, uint64_t key_id, const std::string& api_key,
                     const std::string& endpoint, int64_t weight, bool allowed,
                     const std::string& reason, uint64_t retry_after = 0,
                     const std::string& ip_address = "");
    
    // 批量记录API调用日志（用于异步写入优化）
    bool bulk_log_api_calls(const std::vector<ApiCallLog>& logs);
    
    // 获取客户端的调用日志
    std::vector<ApiCallLog> get_client_logs(uint64_t client_id, uint64_t start_time,
                                          uint64_t end_time, int limit = 100,
                                          int offset = 0);
    
    // 获取API Key的调用日志
    std::vector<ApiCallLog> get_key_logs(uint64_t key_id, uint64_t start_time,
                                       uint64_t end_time, int limit = 100,
                                       int offset = 0);
    
    // 获取客户端在时间范围内的调用统计
    struct CallStats {
        int64_t total_calls = 0;
        int64_t allowed_calls = 0;
        int64_t rejected_calls = 0;
        int64_t total_weight = 0;
    };
    
    // 端点统计数据
    struct EndpointStats {
        uint64_t total_calls;
        uint64_t allowed_calls;
        double avg_response_time;
    };
    
    // 每日统计数据
    struct DailyStats {
        uint64_t total_calls;
        uint64_t allowed_calls;
    };
    
    CallStats get_client_call_stats(uint64_t client_id, uint64_t start_time,
                                  uint64_t end_time);
    
    // 获取当前时间窗口内的调用计数
    int64_t get_client_calls_in_time_window(uint64_t client_id, uint64_t window_start);
    
    // 清理过期日志（保留最近N天）
    int64_t cleanup_old_logs(int days_to_keep = 30);
    
    // 获取端点统计数据
    std::unordered_map<std::string, EndpointStats> get_endpoint_stats(std::optional<std::string> from_date = std::nullopt,
                                                                    std::optional<std::string> to_date = std::nullopt);
    
    // 获取拒绝原因分布
    std::unordered_map<std::string, uint64_t> get_denial_reason_distribution(
        std::optional<uint64_t> client_id = std::nullopt,
        std::optional<std::string> from_date = std::nullopt,
        std::optional<std::string> to_date = std::nullopt);
    
    // 获取最近的API调用日志
    std::vector<ApiCallLog> get_recent_logs(size_t limit = 100);
    
    // 获取每日统计数据
    DailyStats get_daily_stats();
    
    // 获取实时调用统计（用于监控）
    struct RealtimeStats {
        int64_t calls_last_minute = 0;
        int64_t calls_last_hour = 0;
        int64_t rejected_last_minute = 0;
        double avg_response_time = 0.0;
    };
    
    RealtimeStats get_realtime_stats();
    
private:
    std::string database_path_;
    
    // SQLite相关操作（内部实现）
    class SQLiteImpl;
    std::unique_ptr<SQLiteImpl> impl_;
};

} // namespace repository
} // namespace api_quota

#endif // LOG_REPOSITORY_HPP