# 测试目录的CMakeLists.txt

# 添加任务状态流转测试
add_executable(task_status_test task_status_test.cpp)
target_link_libraries(task_status_test ${SQLite3_LIBRARIES})

# 添加任务查询过滤测试
add_executable(task_query_test task_query_test.cpp)
target_link_libraries(task_query_test ${SQLite3_LIBRARIES})

# 添加测试到CTest
add_test(NAME TaskStatusTransitionTests COMMAND task_status_test)
add_test(NAME TaskQueryFilteringTests COMMAND task_query_test)

# 确保在编译测试之前编译主程序的库（如果需要）
target_include_directories(task_status_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(task_query_test PRIVATE ${CMAKE_SOURCE_DIR}/include)