# CMake构建脚本
cmake_minimum_required(VERSION 3.10)
project(RideSharingMatchingSystem VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 查找所需的库
find_package(Threads REQUIRED)

# 定义源代码目录
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 定义头文件目录
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 收集所有源文件
file(GLOB_RECURSE SOURCES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.cc"
)

# 收集所有头文件（用于IDE显示）
file(GLOB_RECURSE HEADERS
    "${SOURCE_DIR}/*.h"
    "${SOURCE_DIR}/*.hpp"
)

# 创建可执行文件
add_executable(ride_sharing_system
    ${SOURCES}
    ${HEADERS}
)

# 包含头文件目录
target_include_directories(ride_sharing_system PRIVATE
    ${INCLUDE_DIR}
)

# 链接线程库
target_link_libraries(ride_sharing_system PRIVATE
    Threads::Threads
)

# 设置输出目录
set_target_properties(ride_sharing_system PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

# 安装规则（可选）
install(TARGETS ride_sharing_system
    RUNTIME DESTINATION bin
)

# 打印构建信息
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source Directory: ${SOURCE_DIR}")
message(STATUS "Include Directory: ${INCLUDE_DIR}")
