cmake_minimum_required(VERSION 3.10)
project(OnlineJudgeBackend)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加头文件目录
include_directories(src/)

# 收集所有源文件
file(GLOB_RECURSE SOURCES src/*.cpp)

# 创建可执行文件
add_executable(online_judge_backend ${SOURCES})

# 链接必要的库
target_link_libraries(online_judge_backend pthread)

# 创建日志目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)