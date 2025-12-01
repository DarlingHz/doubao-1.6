#ifndef SUMMARY_CONTROLLER_H
#define SUMMARY_CONTROLLER_H

#include <string>
#include <memory>
#include "utils/HttpServer.h"
#include "services/SummaryService.h"
#include "utils/JsonUtils.h"

namespace accounting {

class SummaryController {
public:
    SummaryController(std::shared_ptr<SummaryService> summaryService, std::shared_ptr<JsonUtils> jsonUtils);
    
    // 获取月度汇总
    HttpResponse getMonthlySummary(const HttpRequest& req);
    
    // 获取趋势汇总
    HttpResponse getTrendSummary(const HttpRequest& req);
    
    // 清除汇总缓存
    HttpResponse clearCache(const HttpRequest& req);
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    std::shared_ptr<SummaryService> summaryService_;
    std::shared_ptr<JsonUtils> jsonUtils_;
    
    // 处理错误响应
    HttpResponse handleError(const std::string& errorMsg, int statusCode = 400);
};

} // namespace accounting

#endif // SUMMARY_CONTROLLER_H