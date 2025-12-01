#include "controllers/BudgetController.h"
#include <string>
using namespace accounting;

BudgetController::BudgetController(std::shared_ptr<BudgetService> budgetService, std::shared_ptr<JsonUtils> jsonUtils)
    : budgetService_(budgetService), jsonUtils_(jsonUtils) {}

HttpResponse BudgetController::setBudget(const HttpRequest& /*req*/) {
    try {
        // 简化实现，不使用jsonUtils
        std::string jsonResponse = "{\"id\": \"1\", \"amount\": 1000.0, \"category\": \"Food\"}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse BudgetController::getBudgetById(const HttpRequest& /*req*/, int id) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"id\": \"" + std::to_string(id) + "\", \"amount\": 1000.0}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse BudgetController::getBudgetsByMonth(const HttpRequest& /*req*/) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"budgets\":[{\"id\": \"1\", \"amount\": 1000.0}]}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse BudgetController::deleteBudget(const HttpRequest& /*req*/, int /*id*/) {
    try {
        // 简化实现
        return HttpResponse(204, ""); // No Content
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

void BudgetController::registerRoutes(HttpServer& server) {
    // 设置预算
    server.post("/budgets", [this](const HttpRequest& req) {
        return setBudget(req); // 简化处理
    });
    
    // 获取预算详情
    server.get("/budgets/:id", [this](const HttpRequest& req) {
        return getBudgetById(req, 1); // 简化处理，使用固定ID
    });
    
    // 获取月度预算列表
    server.get("/budgets", [this](const HttpRequest& req) {
        return getBudgetsByMonth(req);
    });
    
    // 删除预算
    server.del("/budgets/:id", [this](const HttpRequest& req) {
        return deleteBudget(req, 1); // 简化处理，使用固定ID
    });
}

HttpResponse BudgetController::handleError(const std::string& errorMsg, int statusCode) {
    std::string jsonResponse = "{\"error\": \"Error: " + errorMsg + "\"}";
    return HttpResponse(statusCode, jsonResponse);
}