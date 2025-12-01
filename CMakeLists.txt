cmake_minimum_required(VERSION 3.14)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # 为nlohmann/json库添加最低策略版本
project(ApiQuotaManager)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# 查找SQLite3
find_package(SQLite3 REQUIRED)

# 查找系统线程库
find_package(Threads REQUIRED)

# 项目已包含自己的json实现，不需要外部json库

# 创建目录结构
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)

# 包含头文件目录
include_directories(include)
include_directories(${SQLite3_INCLUDE_DIRS})

# 定义源文件变量
set(SOURCES
    src/http/server.cpp
    src/http/handler.cpp
    src/http/http_request.cpp
    src/http/http_response.cpp
    src/http/http_router.cpp
    src/service/client_service.cpp
    src/service/api_key_service.cpp
    src/service/quota_service.cpp
    src/service/stats_service.cpp
    src/repository/client_repository.cpp
    src/repository/api_key_repository.cpp
    src/repository/log_repository.cpp
    src/repository/db_connection_pool.cpp
    src/config/config.cpp
    src/utils/utils.cpp
    src/utils/json.cpp
    src/main.cpp
)

# 定义头文件变量
set(HEADERS
    include/http/server.hpp
    include/http/handler.hpp
    include/service/client_service.hpp
    include/service/api_key_service.hpp
    include/service/quota_service.hpp
    include/service/stats_service.hpp
    include/repository/client_repository.hpp
    include/repository/api_key_repository.hpp
    include/repository/log_repository.hpp
    include/config/config.hpp
    include/utils/utils.hpp
    include/utils/json.hpp
    include/utils/memory_cache.hpp
)

# 创建可执行文件
add_executable(api-quota-server ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(api-quota-server
    ${SQLite3_LIBRARIES}
    Threads::Threads
)

# 安装目标
install(TARGETS api-quota-server DESTINATION bin)

# 添加测试 - 暂时禁用，因为tests目录中没有CMakeLists.txt文件
option(BUILD_TESTS "Build tests" OFF)
#if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
#    add_subdirectory(tests)
#endif()