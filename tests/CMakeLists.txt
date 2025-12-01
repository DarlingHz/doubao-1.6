# 测试代码构建配置

# 添加Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 设置头文件目录
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# 收集测试源文件
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 创建测试可执行文件
add_executable(test_runner ${TEST_SOURCES})

# 链接依赖
target_link_libraries(test_runner 
    ${GTEST_BOTH_LIBRARIES}
    ${MYSQLCLIENT_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 添加测试
add_test(NAME UnitTests COMMAND test_runner)

# 运行测试的命令
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_runner
    DEPENDS test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "运行单元测试")
