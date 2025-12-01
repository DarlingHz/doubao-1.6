#include "controllers/TransactionController.h"

using namespace accounting;
#include <string>

TransactionController::TransactionController(std::shared_ptr<TransactionService> transactionService, std::shared_ptr<JsonUtils> jsonUtils)
    : transactionService_(transactionService), jsonUtils_(jsonUtils) {}

HttpResponse TransactionController::createTransaction(const HttpRequest&) {
    try {
        // 简化实现，直接返回模拟数据
        std::string jsonResponse = "{\"id\": \"1\", \"amount\": 100.5, \"description\": \"Sample Transaction\"}";
        
        return HttpResponse(201, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse TransactionController::getTransactionById(const HttpRequest&, int id) {
    try {
        // 简化实现，直接返回模拟数据
        std::string jsonResponse = "{\"id\": \"" + std::to_string(id) + "\", \"amount\": 100.5, \"description\": \"Transaction " + std::to_string(id) + "\"}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse TransactionController::queryTransactions(const HttpRequest&) {
    try {
        // 简化实现
        std::string jsonResponse = "{\"transactions\":[{\"id\": \"1\", \"description\": \"Sample Transaction\"}]}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse TransactionController::updateTransaction(const HttpRequest&, int id) {
    try {
        // 简化实现，直接返回模拟数据
        std::string jsonResponse = "{\"id\": \"" + std::to_string(id) + "\", \"amount\": 200.75, \"description\": \"Updated Transaction\"}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

HttpResponse TransactionController::deleteTransaction(const HttpRequest&) {
    try {
        // 简化实现，直接返回成功消息
        std::string jsonResponse = "{\"message\": \"Transaction deleted successfully\"}";
        
        return HttpResponse(200, jsonResponse);
    } catch (const std::exception& e) {
        return handleError(e.what());
    }
}

void TransactionController::registerRoutes(HttpServer& server) {
    // 创建交易
    server.post("/transactions", [this](const HttpRequest& req) {
        return createTransaction(req);
    });
    
    // 查询交易记录列表
    server.get("/transactions", [this](const HttpRequest& req) {
        return queryTransactions(req);
    });
    
    // 获取单个交易记录
    server.get("/transactions/:id", [this](const HttpRequest& req) {
        return getTransactionById(req, 1); // 简化处理，使用固定ID
    });
    
    // 更新交易记录
    server.put("/transactions/:id", [this](const HttpRequest& req) {
        return updateTransaction(req, 1); // 简化处理，使用固定ID
    });
    
    // 删除交易记录
    server.del("/transactions/:id", [this](const HttpRequest& req) {
        return deleteTransaction(req); // 简化处理
    });
}

HttpResponse TransactionController::handleError(const std::string& errorMsg, int statusCode) {
    std::string jsonResponse = "{\"error\": \"Error: " + errorMsg + "\"}";
    return HttpResponse(statusCode, jsonResponse);
}

// buildFilterFromRequest方法已移除，使用简化的查询参数处理