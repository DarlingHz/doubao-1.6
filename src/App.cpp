#include "App.h"
#include <iostream>
#include "utils/HttpServer.h"
#include "dao/Database.h"
#include "dao/AccountDAO.h"
#include "dao/CategoryDAO.h"
#include "dao/TransactionDAO.h"
#include "dao/BudgetDAO.h"
#include "services/AccountService.h"
#include "services/CategoryService.h"
#include "services/TransactionService.h"
#include "services/BudgetService.h"
#include "services/SummaryService.h"
#include "utils/Cache.h"
#include "utils/JsonUtils.h"

App::App() {
    server_ = std::make_shared<HttpServer>();
}

void App::initialize() {
    // 初始化依赖组件
    initializeDependencies();
    
    // 设置中间件
    setupMiddlewares();
    
    // 注册路由
    registerRoutes();
}

void App::run(int port) {
    std::cout << "Starting server on port " << port << "..." << std::endl;
    std::cout << "Server URL: http://localhost:" << port << std::endl;
    server_->start();
}



void App::initializeDependencies() {
    // 初始化数据库
    auto db = std::make_shared<accounting::Database>();
    db->initialize();
    
    // 初始化DAO层
    auto accountDAO = std::make_shared<accounting::AccountDAO>(*db);
    auto categoryDAO = std::make_shared<accounting::CategoryDAO>(*db);
    auto transactionDAO = std::make_shared<accounting::TransactionDAO>(*db);
    auto budgetDAO = std::make_shared<accounting::BudgetDAO>(*db);
    
    // 初始化工具类
    auto cache = std::make_shared<accounting::Cache<std::string, std::string>>();
    auto jsonUtils = std::make_shared<accounting::JsonUtils>();
    
    // 初始化服务层
    auto accountService = std::make_shared<accounting::AccountService>(*db);
    auto categoryService = std::make_shared<accounting::CategoryService>(*db);
    auto transactionService = std::make_shared<accounting::TransactionService>(*db);
    auto budgetService = std::make_shared<accounting::BudgetService>(*db);
    auto summaryService = std::make_shared<accounting::SummaryService>(*db);
    
    // 初始化控制器
    accountController_ = std::make_shared<accounting::AccountController>(accountService, jsonUtils);
    // categoryController_ = std::make_shared<accounting::CategoryController>(categoryService, jsonUtils);
    // transactionController_ = std::make_shared<accounting::TransactionController>(transactionService, jsonUtils);
    // budgetController_ = std::make_shared<accounting::BudgetController>(budgetService, jsonUtils);
    // summaryController_ = std::make_shared<accounting::SummaryController>(summaryService, jsonUtils);
}

void App::registerRoutes() {
    // 注册健康检查路由
    server_->get("/health", [](const HttpRequest&) {
        HttpResponse res(200);
        res.body = "{\"status\":\"ok\"}";
        return res;
    });
    
    // 注册各模块路由
    accountController_->registerRoutes(*server_);
    // 暂时注释掉其他控制器，先验证基本功能
    // categoryController_->registerRoutes(*server_);
    // transactionController_->registerRoutes(*server_);
    // budgetController_->registerRoutes(*server_);
    // summaryController_->registerRoutes(*server_);
}

void App::setupMiddlewares() {
    // 设置错误处理
    server_->setErrorHandler([](const HttpRequest&, const std::exception& ex) {
        HttpResponse res(500);
        res.body = "{\"error\":\"Internal Server Error\",\"message\":\"" + 
                  std::string(ex.what()) + "\"}";
        return res;
    });
}