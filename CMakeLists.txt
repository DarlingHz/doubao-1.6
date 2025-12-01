cmake_minimum_required(VERSION 3.14)
project(MovieWatchServer CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出编译命令到compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -g")

# 添加宏定义，表示使用模拟数据库
add_definitions(-DUSE_MOCK_DATABASE)

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 安装配置文件
install(DIRECTORY config/ DESTINATION config)
install(DIRECTORY db/ DESTINATION db)

# 启用测试
enable_testing()
