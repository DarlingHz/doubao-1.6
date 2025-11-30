cmake_minimum_required(VERSION 3.10)
project(TeamManagementSystem)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找SQLite3库
find_package(SQLite3 REQUIRED)
include_directories(${SQLite3_INCLUDE_DIRS})

# 设置头文件路径
include_directories(include)

# 收集所有源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(team_management_server ${SOURCES})

# 链接SQLite3库
target_link_libraries(team_management_server ${SQLite3_LIBRARIES})

# 检查系统类型，对于不同的系统链接不同的网络库
if(WIN32)
    # Windows系统需要链接ws2_32库用于socket编程
    target_link_libraries(team_management_server ws2_32)
else()
    # Linux/macOS系统需要链接pthread库用于线程
    target_link_libraries(team_management_server pthread)
endif()

# 添加编译选项
if(MSVC)
    # Windows平台的编译选项
    add_compile_options(/W4)
else()
    # Linux/macOS平台的编译选项
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 创建安装目标
install(TARGETS team_management_server DESTINATION bin)
install(FILES db/schema.sql DESTINATION share/team_management)

# 添加测试（如果有）
enable_testing()
add_subdirectory(tests)