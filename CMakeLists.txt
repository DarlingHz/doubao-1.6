cmake_minimum_required(VERSION 3.15)
project(server_monitor_backend CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(SQLite3 REQUIRED)
find_package(Crow REQUIRED)
find_package(Threads REQUIRED)

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${SQLite3_INCLUDE_DIRS}
    ${Crow_INCLUDE_DIRS}
)

# 收集源文件
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# 创建可执行文件
add_executable(server_monitor_backend ${SOURCES})

# 链接库
target_link_libraries(server_monitor_backend
    ${SQLite3_LIBRARIES}
    ${Crow_LIBRARIES}
    Threads::Threads
)

# 单元测试配置
find_package(GTest)
if(GTest_FOUND)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    )
    add_executable(run_tests ${TEST_SOURCES} src/alert/AlertEngine.cpp)
    target_link_libraries(run_tests
        ${SQLite3_LIBRARIES}
        GTest::GTest
        GTest::Main
    )
    add_test(NAME AllTests COMMAND run_tests)
endif()

# 安装配置
install(TARGETS server_monitor_backend DESTINATION bin)
install(DIRECTORY sql/ DESTINATION share/server_monitor_backend/sql)
