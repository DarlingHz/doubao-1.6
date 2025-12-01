// 系统控制器实现
#include "system_controller.h"
#include "../utils/stats_monitor.h"
#include "../utils/logger.h"
#include "../service/user_service.h"
#include <chrono>
#include <ctime>
#include <sstream>

SystemController::SystemController() {
    // 获取统计监控单例
    statsMonitor = StatsMonitor::getInstance();
}

SystemController::~SystemController() {
    // 不需要释放单例
}

void SystemController::initialize() {
    // 注册系统统计相关路由
    registerRoute("GET", "/stats/system", &SystemController::handleGetSystemStats);
    registerRoute("GET", "/stats/performance", &SystemController::handleGetPerformanceStats);
    registerRoute("GET", "/stats/endpoints", &SystemController::handleGetEndpointStats);
    registerRoute("GET", "/stats/active-users", &SystemController::handleGetActiveUsers);
    
    // 注册系统管理相关路由
    registerRoute("GET", "/system/info", &SystemController::handleGetSystemInfo);
    registerRoute("POST", "/system/reset-stats", &SystemController::handleResetStats);
    registerRoute("PUT", "/system/log-level", &SystemController::handleSetLogLevel);
}

// 系统统计相关路由处理函数
HttpResponse SystemController::handleGetSystemStats(const HttpRequest& request) {
    try {
        // 生成系统统计信息JSON
        std::string statsJson = statsMonitor->generateStatsJSON();
        return createSuccessResponse(statsJson);
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting system stats: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get system stats: " + std::string(e.what()));
    }
}

HttpResponse SystemController::handleGetPerformanceStats(const HttpRequest& request) {
    try {
        // 构建性能统计JSON
        std::stringstream ss;
        ss << "{";
        ss << "\"total_requests\":" << statsMonitor->getTotalRequests() << ",";
        ss << "\"total_matches\":" << statsMonitor->getTotalMatches() << ",";
        ss << "\"total_trips\":" << statsMonitor->getTotalTrips() << ",";
        ss << "\"average_response_time\":" << statsMonitor->getAverageResponseTime() << ",";
        ss << "\"max_response_time\":" << statsMonitor->getMaxResponseTime() << ",";
        ss << "\"average_match_time\":" << statsMonitor->getAverageMatchTime() << ",";
        ss << "\"current_qps\":" << statsMonitor->getCurrentQPS() << ",";
        ss << "\"system_uptime\":" << statsMonitor->getSystemUptime() << ",";
        ss << "\"matching_success_rate\":" << statsMonitor->getMatchingSuccessRate();
        ss << "}";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting performance stats: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get performance stats: " + std::string(e.what()));
    }
}

HttpResponse SystemController::handleGetEndpointStats(const HttpRequest& request) {
    try {
        // 获取端点性能统计
        std::vector<EndpointStats> endpointStats = statsMonitor->getEndpointStats();
        
        // 构建JSON响应
        std::stringstream ss;
        ss << "[";
        
        bool first = true;
        for (const auto& stats : endpointStats) {
            if (!first) {
                ss << ",";
            }
            first = false;
            
            ss << "{";
            ss << "\"endpoint\":\"" << stats.endpoint << "\",";
            ss << "\"request_count\":" << stats.requestCount << ",";
            ss << "\"avg_latency\":" << stats.avgLatency << ",";
            ss << "\"max_latency\":" << stats.maxLatency << ",";
            ss << "\"min_latency\":" << stats.minLatency;
            ss << "}";
        }
        
        ss << "]";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting endpoint stats: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get endpoint stats: " + std::string(e.what()));
    }
}

HttpResponse SystemController::handleGetActiveUsers(const HttpRequest& request) {
    try {
        // 构建活跃用户统计JSON
        std::stringstream ss;
        ss << "{";
        ss << "\"online_drivers\":" << statsMonitor->getOnlineDrivers() << ",";
        ss << "\"available_drivers\":" << statsMonitor->getAvailableDrivers() << ",";
        ss << "\"active_riders\":" << statsMonitor->getActiveRiders() << ",";
        ss << "\"pending_requests\":" << statsMonitor->getPendingRequests();
        ss << "}";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting active users: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get active users: " + std::string(e.what()));
    }
}

// 系统管理相关路由处理函数
HttpResponse SystemController::handleGetSystemInfo(const HttpRequest& request) {
    try {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream timeStream;
        timeStream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
        
        // 构建系统信息JSON
        std::stringstream ss;
        ss << "{";
        ss << "\"system_name\":\"Ride-sharing Matching System\",";
        ss << "\"version\":\"1.0.0\",";
        ss << "\"current_time\":\"" << timeStream.str() << "\",";
        ss << "\"uptime_seconds\":" << statsMonitor->getSystemUptime() << ",";
        ss << "\"log_level\":\"" << Logger::getInstance()->getCurrentLogLevel() << "\",";
        ss << "\"thread_count\":" << std::thread::hardware_concurrency();
        ss << "}";
        
        return createSuccessResponse(ss.str());
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error getting system info: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to get system info: " + std::string(e.what()));
    }
}

HttpResponse SystemController::handleResetStats(const HttpRequest& request) {
    try {
        // 重置性能统计
        statsMonitor->resetPerformanceStats();
        
        Logger::getInstance()->info("Performance statistics have been reset");
        return createSuccessResponse("{\"message\":\"Performance statistics have been reset successfully\"}");
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error resetting stats: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to reset statistics: " + std::string(e.what()));
    }
}

HttpResponse SystemController::handleSetLogLevel(const HttpRequest& request) {
    // 解析JSON请求体
    auto data = parseJsonBody(request.body);
    
    if (data.find("log_level") == data.end()) {
        return createErrorResponse(400, "Log level is required");
    }
    
    try {
        std::string logLevel = data["log_level"];
        
        // 设置日志级别
        bool success = Logger::getInstance()->setLogLevel(logLevel);
        
        if (success) {
            Logger::getInstance()->info("Log level has been set to " + logLevel);
            return createSuccessResponse("{\"message\":\"Log level has been set to " + logLevel + "\"}");
        } else {
            return createErrorResponse(400, "Invalid log level. Supported levels: DEBUG, INFO, WARNING, ERROR");
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error setting log level: " + std::string(e.what()));
        return createErrorResponse(500, "Failed to set log level: " + std::string(e.what()));
    }
}
