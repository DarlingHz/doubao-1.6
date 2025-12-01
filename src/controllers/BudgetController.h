#ifndef BUDGET_CONTROLLER_H
#define BUDGET_CONTROLLER_H

#include <string>
#include <memory>
#include "utils/HttpServer.h"
#include "services/BudgetService.h"
#include "utils/JsonUtils.h"

namespace accounting {

class BudgetController {
public:
    BudgetController(std::shared_ptr<BudgetService> budgetService, std::shared_ptr<JsonUtils> jsonUtils);
    
    // 设置或更新预算
    HttpResponse setBudget(const HttpRequest& req);
    
    // 获取预算详情
    HttpResponse getBudgetById(const HttpRequest& req, int id);
    
    // 获取某月的预算列表
    HttpResponse getBudgetsByMonth(const HttpRequest& req);
    
    // 删除预算
    HttpResponse deleteBudget(const HttpRequest& req, int id);
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    std::shared_ptr<BudgetService> budgetService_;
    std::shared_ptr<JsonUtils> jsonUtils_;
    
    // 处理错误响应
    HttpResponse handleError(const std::string& errorMsg, int statusCode = 400);
};

} // namespace accounting

#endif // BUDGET_CONTROLLER_H