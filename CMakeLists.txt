cmake_minimum_required(VERSION 3.14)
project(shortlink_service CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型设置
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# 设置编译选项
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 查找系统依赖库
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# 尝试查找 PkgConfig
find_package(PkgConfig QUIET)

# 尝试查找 GTest
find_package(GTest QUIET)

# 如果找不到 GTest，跳过测试
if(NOT GTest_FOUND)
    message(WARNING "GoogleTest not found, tests will be skipped")
    set(BUILD_TESTS OFF)
endif()

# 添加本地依赖库的包含路径
# 假设这些头文件已经在 include 目录下
include_directories(${CMAKE_SOURCE_DIR}/include)

# 包含目录
include_directories(include)

# 源文件收集
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(shortlink_server ${SOURCES})

# 链接依赖
target_link_libraries(shortlink_server PRIVATE
  SQLite::SQLite3
  Threads::Threads
)

# 单元测试
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES "tests/*.cpp")
    add_executable(shortlink_tests ${TEST_SOURCES})
    target_link_libraries(shortlink_tests PRIVATE
      GTest::gtest_main
      SQLite::SQLite3
      Threads::Threads
    )

    # 测试发现
    include(GoogleTest)
    gtest_discover_tests(shortlink_tests)
endif()

# 复制配置文件到构建目录
configure_file(config/config.json config/config.json COPYONLY)
