# 定义主程序名称
set(TARGET_NAME watch_server)

# 设置源文件列表
set(SOURCES
    main.cpp
    model/User.cpp
    model/Movie.cpp
    model/WatchRecord.cpp
    repository/Database.cpp
    repository/UserRepository.cpp
    repository/MovieRepository.cpp
    repository/WatchRecordRepository.cpp
    service/UserService.cpp
    service/MovieService.cpp
    service/WatchRecordService.cpp
    service/StatisticsService.cpp
    service/RecommendationService.cpp
    http/Server.cpp
    http/RequestHandler.cpp
    http/Router.cpp
    http/HttpUtils.cpp
    utils/Config.cpp
    utils/Logger.cpp
    utils/Cache.cpp
    utils/ConnectionPool.cpp
    utils/TimeUtils.cpp
)

# 设置头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 添加可执行文件
add_executable(${TARGET_NAME} ${SOURCES})

# 链接库
target_link_libraries(${TARGET_NAME}
    pthread
)

# 安装可执行文件
install(TARGETS ${TARGET_NAME} DESTINATION bin)
