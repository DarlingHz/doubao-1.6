#include <iostream>
#include <memory>
#include <thread>
#include <signal.h>
#include <chrono>
#include "utils/Config.h"
#include "utils/Logger.h"
#include "repository/Database.h"
#include "repository/UserRepository.h"
#include "repository/MovieRepository.h"
#include "repository/WatchRecordRepository.h"
#include "service/UserService.h"
#include "service/MovieService.h"
#include "service/WatchRecordService.h"
#include "service/StatisticsService.h"
#include "service/RecommendationService.h"
#include "http/Server.h"
#include "http/RequestHandler.h"

std::unique_ptr<http::Server> g_server;

void signalHandler(int signum) {
    std::cout << "收到信号 " << signum << "，正在关闭服务器..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    try {
        // 注册信号处理
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // 初始化数据库连接
        auto database = std::make_shared<repository::Database>();
        if (!database->open("./database.db")) { // 使用本地数据库文件并检查连接
            std::cerr << "数据库连接失败！" << std::endl;
            return 1;
        }

        // 创建Repository实例
        auto userRepository = std::make_shared<repository::UserRepository>(database);
        auto movieRepository = std::make_shared<repository::MovieRepository>(database);
        auto watchRecordRepository = std::make_shared<repository::WatchRecordRepository>(database);

        // 创建Service实例
        auto userService = std::make_shared<service::UserService>(userRepository);
        auto movieService = std::make_shared<service::MovieService>(movieRepository);
        auto watchRecordService = std::make_shared<service::WatchRecordService>(
            watchRecordRepository, userRepository, movieRepository
        );
        auto statisticsService = std::make_shared<service::StatisticsService>(
            watchRecordRepository, userRepository, std::shared_ptr<utils::Cache>(&utils::Cache::getInstance(), [](void*){}) // 使用空删除器
        );
        auto recommendationService = std::make_shared<service::RecommendationService>(
            movieRepository, watchRecordRepository, userRepository
        );

        // 获取配置和日志记录器的单例实例并创建共享指针包装
        auto config = std::shared_ptr<utils::Config>(&utils::Config::getInstance(), [](utils::Config*) {});
        auto logger = std::shared_ptr<utils::Logger>(&utils::Logger::getInstance(), [](utils::Logger*) {});
        
        // 加载配置文件并设置服务器端口
        config->loadFromFile("config.ini");
        int serverPort = config->getInt("server.port", 8080);
        
        // 创建请求处理器
        auto requestHandler = std::make_shared<http::RequestHandler>(
            userService, movieService, watchRecordService, statisticsService, recommendationService, logger
        );

        // 创建并启动HTTP服务器
        g_server = std::make_unique<http::Server>(config, logger, requestHandler);
        std::cout << "服务器正在启动，监听端口 8080..." << std::endl;
        g_server->start();

        // 保持主线程运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        std::cerr << "发生异常：" << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "发生未知异常！" << std::endl;
        return 1;
    }

    return 0;
}