cmake_minimum_required(VERSION 3.10)
project(InventoryOrderManagement)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(Threads REQUIRED)

# 使用系统 SQLite3（通常在 macOS 上已预装）
include_directories(/usr/include)
set(SQLITE3_LIBRARY sqlite3)

# 头文件目录
include_directories(include ${SQLITE3_INCLUDE_DIRS})

# 源文件列表
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/product_service.cpp
    src/order_service.cpp
    src/stats_service.cpp
    src/database.cpp
    src/request_handler.cpp
    src/utils.cpp
)

# 创建可执行文件
add_executable(inventory_server ${SOURCES})

# 链接依赖
target_link_libraries(inventory_server
    ${SQLITE3_LIBRARY}
    Threads::Threads
)

# 编译选项
if(MSVC)
    target_compile_options(inventory_server PRIVATE /W4)
else()
    target_compile_options(inventory_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 确保测试脚本可执行
file(COPY tests/test.sh DESTINATION ${CMAKE_BINARY_DIR})
file(COPY sql/schema.sql DESTINATION ${CMAKE_BINARY_DIR}/sql)

# 安装规则
install(TARGETS inventory_server DESTINATION bin)
install(DIRECTORY sql/ DESTINATION share/inventory_server/sql)
install(PROGRAMS tests/test.sh DESTINATION share/inventory_server/tests)
