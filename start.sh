#!/bin/bash

# 顺风车智能匹配系统启动脚本

echo "============================================"
echo "        Ride-sharing Matching System         "
echo "============================================"

# 检查是否已存在构建目录
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# 进入构建目录
cd build

# 检查是否需要重新配置
if [ ! -f "Makefile" ]; then
    echo "Configuring project with CMake..."
    cmake ..
    if [ $? -ne 0 ]; then
        echo "CMake configuration failed!"
        exit 1
    fi
fi

# 编译项目
echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# 创建日志目录
mkdir -p logs

# 启动服务器
echo "============================================"
echo "Starting Ride-sharing Matching System..."
echo "Server will be available at http://localhost:8000"
echo "Press Ctrl+C to stop the server"
echo "============================================"

exec ./bin/ride_sharing_system $1 $2
