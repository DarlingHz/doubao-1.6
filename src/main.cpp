#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "common/config.h"
#include "common/log.h"
#include "common/error_code.h"
#include "common/simple_password_hasher.h"
#include "auth/simple_token_manager.h"
#include "models/memory_user_repository.h"
#include "models/memory_problem_repository.h"
#include "models/memory_submission_repository.h"
#include "services/user_service.h"
#include "services/problem_service.h"
#include "services/submission_service.h"

int main(int argc, char* argv[]) {
    // 初始化配置模块
    if (!Config::Instance().Load("config/config.json")) {
        std::cerr << "Failed to load config file" << std::endl;
        return 1;
    }

    // 初始化日志模块
    std::string log_file_path = Config::Instance().GetString("log_file_path", "logs/app.log");
    if (!Log::Instance().Init(log_file_path)) {
        std::cerr << "Failed to initialize log system" << std::endl;
        return 1;
    }

    // 初始化错误码模块
    ErrorCodeManager::Instance().Init();

    // 初始化密码哈希器
    auto password_hasher = std::make_shared<SimplePasswordHasher>();

    // 初始化令牌管理器
    int token_expiration_hours = Config::Instance().GetInt("token_expiration_hours", 24);
    auto token_manager = std::make_shared<SimpleTokenManager>(std::chrono::hours(token_expiration_hours));

    // 初始化用户仓库
    auto user_repository = std::make_shared<MemoryUserRepository>();

    // 初始化题目仓库
    auto problem_repository = std::make_shared<MemoryProblemRepository>();

    // 初始化提交记录仓库
    auto submission_repository = std::make_shared<MemorySubmissionRepository>();

    // 初始化用户服务
    auto user_service = std::make_shared<UserService>(user_repository, password_hasher, token_manager);

    // 初始化题目服务
    auto problem_service = std::make_shared<ProblemService>(problem_repository);

    // 初始化提交记录服务
    auto submission_service = std::make_shared<SubmissionService>(submission_repository, problem_repository);

    // 启动HTTP服务器
    int port = Config::Instance().GetInt("port", 8080);
    std::cout << "Starting server on port " << port << std::endl;
    LOG_INFO("Starting server on port " + std::to_string(port));

    // TODO: 实现HTTP服务器和路由注册

    // 等待服务器停止
    std::cout << "Server is running on port " << port << std::endl;
    LOG_INFO("Server is running on port " + std::to_string(port));

    // 无限循环保持程序运行
    while (true) {
        // 这里可以添加服务器的事件处理逻辑
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}