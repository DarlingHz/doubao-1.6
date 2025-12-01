# 定义测试程序名称
set(TEST_NAME watch_server_tests)

# 查找GoogleTest（如果需要测试，取消注释下面一行）
# find_package(GTest REQUIRED)

# 如果没有测试文件，暂时不创建测试可执行文件
# 这样可以避免编译错误

# 如果将来有测试文件，可以取消注释下面的代码
# 设置测试源文件列表
# set(TEST_SOURCES
#     test_statistics.cpp
#     test_cache.cpp
#     test_recommendation.cpp
#     test_utils.cpp
# )

# 添加测试可执行文件
# add_executable(${TEST_NAME} ${TEST_SOURCES})

# 链接库
# target_link_libraries(${TEST_NAME}
#     ${GTest_LIBRARIES}
#     GTest::gtest_main
#     pthread
# )

# 包含源文件目录，以便测试可以访问头文件
# include_directories(
#     ${CMAKE_SOURCE_DIR}/src
#     ${GTEST_INCLUDE_DIRS}
# )

# 添加测试
# gtest_discover_tests(${TEST_NAME})

# 安装测试程序（可选）
# install(TARGETS ${TEST_NAME} DESTINATION bin)
