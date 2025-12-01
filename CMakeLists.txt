cmake_minimum_required(VERSION 3.14)
project(PersonalKnowledgeCards VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2")

# 如果是Debug模式，添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# 添加头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# 查找第三方依赖

# 检查是否存在nlohmann/json库
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "未找到nlohmann/json库，请安装")
else()
    include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# 检查是否存在httplib库
find_path(HTTPLIB_INCLUDE_DIR httplib.h)
if(NOT HTTPLIB_INCLUDE_DIR)
    message(FATAL_ERROR "未找到httplib库，请安装")
else()
    include_directories(${HTTPLIB_INCLUDE_DIR})
endif()

# 检查是否存在MySQL客户端库
find_library(MYSQLCLIENT_LIBRARY mysqlclient)
if(NOT MYSQLCLIENT_LIBRARY)
    message(FATAL_ERROR "未找到MySQL客户端库，请安装")
endif()

# 检查是否存在OpenSSL库
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "未找到OpenSSL库，请安装")
endif()

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 构建信息输出
message(STATUS "项目: PersonalKnowledgeCards ${CMAKE_PROJECT_VERSION}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "安装路径: ${CMAKE_INSTALL_PREFIX}")