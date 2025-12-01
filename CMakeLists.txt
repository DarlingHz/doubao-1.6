cmake_minimum_required(VERSION 3.14)
project(AccountingSystem VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 查找SQLite3
find_package(SQLite3 REQUIRED)

# 暂时移除Boost依赖，使用标准库和nlohmann/json
# 查找Boost库用于HTTP服务器和JSON (暂时注释掉)
# find_package(Boost REQUIRED COMPONENTS system thread json)

# 添加可执行文件
add_executable(accounting_server
    src/main.cpp
    src/App.cpp
    src/models/Account.cpp
    src/models/Category.cpp
    src/models/Transaction.cpp
    src/models/Budget.cpp
    # src/models/Summary.cpp (不需要实现文件，因为Summary.h只包含结构体定义)
    src/dao/Database.cpp
    src/dao/AccountDAO.cpp
    src/dao/CategoryDAO.cpp
    src/dao/TransactionDAO.cpp
    src/dao/BudgetDAO.cpp
    src/services/AccountService.cpp
    src/services/CategoryService.cpp
    src/services/TransactionService.cpp
    src/services/BudgetService.cpp
    src/services/SummaryService.cpp
    src/controllers/AccountController.cpp
    src/controllers/CategoryController.cpp
    src/controllers/TransactionController.cpp
    src/controllers/BudgetController.cpp
    src/controllers/SummaryController.cpp
    src/utils/Cache.cpp
    src/utils/HttpServer.cpp
    src/utils/JsonUtils.cpp
    # src/utils/TimeUtils.cpp (不存在，使用标准库日期时间功能)
)

# 包含头文件目录
target_include_directories(accounting_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${SQLite3_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(accounting_server PRIVATE
    SQLite::SQLite3
    # ${Boost_LIBRARIES}
)

# 添加测试 (暂时注释掉，因为测试文件不存在)
# find_package(GTest REQUIRED)
# add_executable(accounting_tests
#    tests/SummaryServiceTest.cpp
#    src/models/Account.cpp
#    src/models/Category.cpp
#    src/models/Transaction.cpp
#    src/models/Budget.cpp
#    src/models/Summary.cpp
#    src/services/SummaryService.cpp
#    src/utils/TimeUtils.cpp
# )

# 测试目标的包含目录 (暂时注释掉)
# target_include_directories(accounting_tests PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/src
#    ${GTEST_INCLUDE_DIRS}
# )

# 链接测试库 (暂时注释掉)
# target_link_libraries(accounting_tests PRIVATE
#    ${GTEST_LIBRARIES}
#    ${GTEST_MAIN_LIBRARIES}
#    pthread
# )

# 启用测试 (暂时注释掉)
# enable_testing()
# add_test(NAME accounting_tests COMMAND accounting_tests)

# 安装配置
install(TARGETS accounting_server DESTINATION bin)

# 提供示例配置文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json.example
    ${CMAKE_CURRENT_BINARY_DIR}/config.json
    COPYONLY
)
