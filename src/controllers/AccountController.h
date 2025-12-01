#ifndef ACCOUNT_CONTROLLER_H
#define ACCOUNT_CONTROLLER_H

#include <string>
#include <memory>
#include "utils/HttpServer.h"
#include "services/AccountService.h"
#include "utils/JsonUtils.h"

namespace accounting {

class AccountController {
public:
    AccountController(std::shared_ptr<AccountService> accountService, std::shared_ptr<JsonUtils> jsonUtils);
    
    // 创建账户
    HttpResponse createAccount(const HttpRequest& req);
    
    // 获取账户详情
    HttpResponse getAccountById(const HttpRequest& req, int id);
    
    // 获取账户列表
    HttpResponse getAccounts(const HttpRequest& req);
    
    // 更新账户
    HttpResponse updateAccount(const HttpRequest& req, int id);
    
    // 删除账户
    HttpResponse deleteAccount(const HttpRequest& req, int id);
    
    // 注册路由
    void registerRoutes(HttpServer& server);
    
private:
    std::shared_ptr<AccountService> accountService_;
    std::shared_ptr<JsonUtils> jsonUtils_;
    
    // 处理错误响应
    HttpResponse handleError(const std::string& errorMsg, int statusCode = 400);
};

} // namespace accounting

#endif // ACCOUNT_CONTROLLER_H