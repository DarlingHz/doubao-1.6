#ifndef TRANSACTION_CONTROLLER_H
#define TRANSACTION_CONTROLLER_H

#include <string>
#include <memory>
#include "utils/HttpServer.h"
#include "services/TransactionService.h"
#include "utils/JsonUtils.h"

namespace accounting {

class TransactionController {
public:
    TransactionController(std::shared_ptr<TransactionService> transactionService, std::shared_ptr<JsonUtils> jsonUtils);
    
    // 创建交易记录
    HttpResponse createTransaction(const HttpRequest& req);
    
    // 获取交易记录详情
    HttpResponse getTransactionById(const HttpRequest& req, int id);
    
    // 查询交易记录列表
    HttpResponse queryTransactions(const HttpRequest& req);
    
    // 更新交易记录
    HttpResponse updateTransaction(const HttpRequest& req, int id);
    
    // 删除交易记录
    HttpResponse deleteTransaction(const HttpRequest& req);
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    std::shared_ptr<TransactionService> transactionService_;
    std::shared_ptr<JsonUtils> jsonUtils_;
    
    // 处理错误响应
    HttpResponse handleError(const std::string& errorMsg, int statusCode = 400);
    
    // 从请求参数构建查询过滤器
    TransactionFilter buildFilterFromRequest(const HttpRequest& req);
};

} // namespace accounting

#endif // TRANSACTION_CONTROLLER_H