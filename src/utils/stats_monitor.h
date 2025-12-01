// 统计和性能监控类
#ifndef STATS_MONITOR_H
#define STATS_MONITOR_H

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

class StatsMonitor {
private:
    static StatsMonitor* instance;
    
    // 系统统计信息
    std::atomic<int> onlineDriversCount;        // 在线车主数量
    std::atomic<int> availableDriversCount;     // 可用车主数量
    std::atomic<int> pendingRequestsCount;      // 待匹配请求数量
    std::atomic<int> activeTripsCount;          // 活跃行程数量
    
    // 性能统计
    std::atomic<long> totalRequestsHandled;     // 总处理请求数
    std::atomic<long> totalMatchesMade;         // 总匹配成功数
    std::atomic<long> totalMatchTime;           // 总匹配耗时（毫秒）
    std::atomic<long> totalMatchDistance;       // 总匹配距离
    
    // 请求延迟统计
    struct RequestLatencyStats {
        std::atomic<long> count;
        std::atomic<long> totalTime;
        std::atomic<long> maxTime;
        
        RequestLatencyStats() : count(0), totalTime(0), maxTime(0) {}
    };
    
    std::unordered_map<std::string, RequestLatencyStats> endpointStats;
    std::mutex endpointStatsMutex;
    
    // 启动时间
    std::chrono::system_clock::time_point startTime;
    
    StatsMonitor();
    
public:
    static StatsMonitor* getInstance();
    
    // 系统状态更新方法
    void incrementOnlineDrivers() { onlineDriversCount++; }
    void decrementOnlineDrivers() { onlineDriversCount--; }
    void incrementAvailableDrivers() { availableDriversCount++; }
    void decrementAvailableDrivers() { availableDriversCount--; }
    void incrementPendingRequests() { pendingRequestsCount++; }
    void decrementPendingRequests() { pendingRequestsCount--; }
    void incrementActiveTrips() { activeTripsCount++; }
    void decrementActiveTrips() { activeTripsCount--; }
    
    // 性能统计更新方法
    void incrementTotalRequests() { totalRequestsHandled++; }
    void recordMatchSuccess(long matchTimeMs, int distance) {
        totalMatchesMade++;
        totalMatchTime += matchTimeMs;
        totalMatchDistance += distance;
    }
    
    // 记录请求延迟
    void recordRequestLatency(const std::string& endpoint, long latencyMs);
    
    // 获取系统状态统计
    int getOnlineDriversCount() const { return onlineDriversCount; }
    int getAvailableDriversCount() const { return availableDriversCount; }
    int getPendingRequestsCount() const { return pendingRequestsCount; }
    int getActiveTripsCount() const { return activeTripsCount; }
    
    // 获取性能统计
    long getTotalRequestsHandled() const { return totalRequestsHandled; }
    long getTotalMatchesMade() const { return totalMatchesMade; }
    double getAverageMatchTime() const {
        return totalMatchesMade > 0 ? static_cast<double>(totalMatchTime) / totalMatchesMade : 0;
    }
    double getAverageMatchDistance() const {
        return totalMatchesMade > 0 ? static_cast<double>(totalMatchDistance) / totalMatchesMade : 0;
    }
    
    // 获取请求延迟统计
    double getAverageRequestLatency(const std::string& endpoint) const;
    long getMaxRequestLatency(const std::string& endpoint) const;
    
    // 获取系统运行时间（秒）
    long getUptimeSeconds() const;
    
    // 获取QPS（每秒查询数）
    double getCurrentQPS() const;
    
    // 生成统计信息JSON字符串
    std::string generateStatsJSON() const;
    
    // 重置统计信息（保留系统状态）
    void resetPerformanceStats();
};

#endif // STATS_MONITOR_H
