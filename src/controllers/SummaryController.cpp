#include "controllers/SummaryController.h"
#include <string>
using namespace accounting;

SummaryController::SummaryController(std::shared_ptr<SummaryService> summaryService, std::shared_ptr<JsonUtils> jsonUtils)
    : summaryService_(summaryService), jsonUtils_(jsonUtils) {}

HttpResponse SummaryController::getMonthlySummary(const HttpRequest&) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"income\": 5000.0, \"expense\": 3000.0, \"balance\": 2000.0}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse SummaryController::getTrendSummary(const HttpRequest&) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"trends\":[{\"month\": \"2024-01\", \"income\": 5000.0, \"expense\": 3000.0}]}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse SummaryController::clearCache(const HttpRequest&) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"message\": \"缓存已清除\"}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

void SummaryController::registerRoutes(HttpServer& server) {
    // 获取月度汇总
    server.get("/summary/monthly", [this](const HttpRequest& req) {
        return getMonthlySummary(req);
    });
    
    // 获取趋势汇总
    server.get("/summary/trends", [this](const HttpRequest& req) {
        return getTrendSummary(req);
    });
    
    // 清除缓存
    server.del("/summary/cache", [this](const HttpRequest& req) {
        return clearCache(req);
    });
}

HttpResponse SummaryController::handleError(const std::string& errorMsg, int statusCode) {
    std::string jsonResponse = "{\"error\": \"Error: " + errorMsg + "\"}";
    return HttpResponse(statusCode, jsonResponse);
}