# 主要源代码构建配置

# 设置头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 收集所有源文件
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

# 创建可执行文件
add_executable(server ${SOURCES})

# 链接依赖库
target_link_libraries(server
    ${MYSQLCLIENT_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# 如果在Linux系统上，还需要链接pthread
if(UNIX AND NOT APPLE)
    target_link_libraries(server pthread)
endif()

# 安装目标
install(TARGETS server DESTINATION bin)
