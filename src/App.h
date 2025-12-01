#ifndef APP_H
#define APP_H

#include <memory>
#include "utils/HttpServer.h"
#include "controllers/AccountController.h"
#include "controllers/CategoryController.h"
#include "controllers/TransactionController.h"
#include "controllers/BudgetController.h"
#include "controllers/SummaryController.h"

class App {
public:
    App();
    
    // 初始化应用
    void initialize();
    
    // 启动服务器
    void run(int port = 8080);
    
private:
    // 应用实例
    std::shared_ptr<HttpServer> server_;
    std::shared_ptr<accounting::Database> database_;
    std::shared_ptr<accounting::JsonUtils> jsonUtils_;
    
    // 控制器实例
    std::shared_ptr<accounting::AccountController> accountController_;
    std::shared_ptr<accounting::CategoryController> categoryController_;
    std::shared_ptr<accounting::TransactionController> transactionController_;
    std::shared_ptr<accounting::BudgetController> budgetController_;
    std::shared_ptr<accounting::SummaryController> summaryController_;
    
    // 初始化依赖组件
    void initializeDependencies();
    
    // 注册所有路由
    void registerRoutes();
    
    // 设置中间件
    void setupMiddlewares();
};

#endif // APP_H