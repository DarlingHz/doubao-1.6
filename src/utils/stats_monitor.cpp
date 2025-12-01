// 统计和性能监控类实现
#include "stats_monitor.h"
#include "logger.h"
#include <sstream>
#include <iomanip>

StatsMonitor* StatsMonitor::instance = nullptr;

StatsMonitor::StatsMonitor() : 
    onlineDriversCount(0),
    availableDriversCount(0),
    pendingRequestsCount(0),
    activeTripsCount(0),
    totalRequestsHandled(0),
    totalMatchesMade(0),
    totalMatchTime(0),
    totalMatchDistance(0),
    startTime(std::chrono::system_clock::now()) {
    Logger::getInstance()->info("StatsMonitor initialized");
}

StatsMonitor* StatsMonitor::getInstance() {
    static std::mutex mutex;
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new StatsMonitor();
        }
    }
    return instance;
}

void StatsMonitor::recordRequestLatency(const std::string& endpoint, long latencyMs) {
    std::lock_guard<std::mutex> lock(endpointStatsMutex);
    auto& stats = endpointStats[endpoint];
    stats.count++;
    stats.totalTime += latencyMs;
    if (latencyMs > stats.maxTime) {
        stats.maxTime = latencyMs;
    }
}

double StatsMonitor::getAverageRequestLatency(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(endpointStatsMutex);
    auto it = endpointStats.find(endpoint);
    if (it != endpointStats.end() && it->second.count > 0) {
        return static_cast<double>(it->second.totalTime) / it->second.count;
    }
    return 0.0;
}

long StatsMonitor::getMaxRequestLatency(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(endpointStatsMutex);
    auto it = endpointStats.find(endpoint);
    if (it != endpointStats.end()) {
        return it->second.maxTime;
    }
    return 0;
}

long StatsMonitor::getUptimeSeconds() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    return duration.count();
}

double StatsMonitor::getCurrentQPS() const {
    long uptime = getUptimeSeconds();
    if (uptime > 0) {
        return static_cast<double>(totalRequestsHandled) / uptime;
    }
    return 0.0;
}

void StatsMonitor::resetPerformanceStats() {
    std::lock_guard<std::mutex> lock(endpointStatsMutex);
    totalRequestsHandled = 0;
    totalMatchesMade = 0;
    totalMatchTime = 0;
    totalMatchDistance = 0;
    
    endpointStats.clear();
    
    Logger::getInstance()->info("Performance statistics reset");
}

double StatsMonitor::getAverageMatchTime() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (totalMatchesMade > 0) {
        return static_cast<double>(totalMatchTime) / totalMatchesMade;
    }
    return 0.0;
}

double StatsMonitor::getAverageMatchDistance() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (totalMatchesMade > 0) {
        return static_cast<double>(totalMatchDistance) / totalMatchesMade;
    }
    return 0.0;
}

std::string StatsMonitor::generateStatsJSON() const {
    std::stringstream ss;
    ss << "{";
    
    // 系统状态
    ss << "\"system\":{";
    ss << "\"online_drivers\":" << onlineDriversCount << ",";
    ss << "\"available_drivers\":" << availableDriversCount << ",";
    ss << "\"pending_requests\":" << pendingRequestsCount << ",";
    ss << "\"active_trips\":" << activeTripsCount << ",";
    ss << "\"uptime_seconds\":" << getUptimeSeconds() << ",";
    ss << "\"qps\":" << std::fixed << std::setprecision(2) << getCurrentQPS();
    ss << "},";
    
    // 性能统计
    ss << "\"performance\":{";
    ss << "\"total_requests\":" << totalRequestsHandled << ",";
    ss << "\"total_matches\":" << totalMatchesMade << ",";
    ss << "\"avg_match_time_ms\":" << std::fixed << std::setprecision(2) << getAverageMatchTime() << ",";
    ss << "\"avg_match_distance\":" << std::fixed << std::setprecision(2) << getAverageMatchDistance();
    ss << "}";
    
    // 端点性能统计
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(endpointStatsMutex));
        if (!endpointStats.empty()) {
            ss << ",\"endpoints\":{";
            bool first = true;
            for (const auto& [endpoint, stats] : endpointStats) {
                if (!first) {
                    ss << ",";
                }
                first = false;
                ss << "\"" << endpoint << "\":{";
                ss << "\"count\":" << stats.count << ",";
                ss << "\"avg_latency_ms\":" << std::fixed << std::setprecision(2) << 
                   (stats.count > 0 ? static_cast<double>(stats.totalTime) / stats.count : 0) << ",";
                ss << "\"max_latency_ms\":" << stats.maxTime;
                ss << "}";
            }
            ss << "}";
        }
    }
    ss << "}";
    return ss.str();
}