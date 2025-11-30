# 在线刷题记录与统计后端服务

这是一个基于C++17的在线刷题记录与统计后端服务，提供RESTful API接口，支持用户注册、登录、题目管理、刷题记录和统计等功能。

## 技术栈

- **C++17**：核心开发语言
- **CMake**：项目构建工具
- **Pthread**：多线程支持
- **内存存储**：数据存储（可扩展为真实数据库）

## 项目结构

```
├── config/              # 配置文件目录
│   └── config.json      # 服务器配置文件
├── logs/                 # 日志文件目录
├── src/                  # 核心业务代码
│   ├── auth/             # 鉴权与token处理
│   ├── common/           # 配置、日志、错误码等公用模块
│   ├── controllers/      # 各REST接口的控制器（待实现）
│   ├── models/           # 实体定义与数据访问层
│   ├── services/         # 业务逻辑与统计计算
│   └── main.cpp          # 程序入口
├── CMakeLists.txt        # CMake构建文件
└── README.md              # 项目说明文档
```

## 功能模块

### 1. 用户与鉴权
- 用户注册
- 用户登录
- 用户信息获取
- Token验证

### 2. 题目管理
- 题目创建
- 题目详情获取
- 题目更新
- 题目删除（软删除）
- 题目列表分页查询
- 题目搜索与过滤

### 3. 刷题记录与统计
- 提交记录创建
- 提交记录详情获取
- 用户提交记录分页查询
- 用户刷题统计信息

## 配置文件

配置文件位于`config/config.json`，包含以下配置项：

```json
{
  "port": 8080,                    # 服务器端口
  "token_expiration_hours": 24,   # Token过期时间（小时）
  "log_file_path": "logs/app.log" # 日志文件路径
}
```

## 编译与运行

### 编译

```bash
mkdir build
cd build
cmake ..
make
```

### 运行

```bash
./online_judge_backend
```

## API接口

### 用户与鉴权

- **注册用户**：`POST /api/v1/users/register`
  - 请求体：`{ "username": "...", "password": "..." }`
  - 响应：用户信息

- **用户登录**：`POST /api/v1/users/login`
  - 请求体：`{ "username": "...", "password": "..." }`
  - 响应：Token

- **获取当前用户信息**：`GET /api/v1/users/me`
  - 请求头：`Authorization: Bearer <token>`
  - 响应：用户信息

### 题目管理

- **创建题目**：`POST /api/v1/problems`
  - 请求体：`{ "title": "...", "description": "...", "difficulty": "...", "tags": ["..."] }`
  - 响应：题目信息

- **获取题目详情**：`GET /api/v1/problems/{id}`
  - 响应：题目信息

- **更新题目**：`PUT /api/v1/problems/{id}`
  - 请求体：`{ "title": "...", "description": "...", "difficulty": "...", "tags": ["..."] }`
  - 响应：题目信息

- **删除题目**：`DELETE /api/v1/problems/{id}`
  - 响应：成功/失败

- **获取题目列表**：`GET /api/v1/problems`
  - 参数：`page`（页码）、`page_size`（每页数量）、`keyword`（关键字）、`difficulty`（难度）、`tags`（标签）
  - 响应：题目列表

- **搜索题目**：`GET /api/v1/problems/search`
  - 参数：`keyword`（关键字）、`difficulty`（难度）、`tags`（标签）、`page`（页码）、`page_size`（每页数量）
  - 响应：题目列表

### 刷题记录与统计

- **创建提交记录**：`POST /api/v1/problems/{id}/records`
  - 请求头：`Authorization: Bearer <token>`
  - 请求体：`{ "status": "...", "time_spent_seconds": ..., "note": "..." }`
  - 响应：提交记录信息

- **获取提交记录详情**：`GET /api/v1/records/{id}`
  - 响应：提交记录信息

- **获取用户提交记录**：`GET /api/v1/users/{id}/records`
  - 参数：`page`（页码）、`page_size`（每页数量）、`problem_id`（题目ID）、`status`（状态）、`start_time`（开始时间）、`end_time`（结束时间）
  - 响应：提交记录列表

- **获取用户统计信息**：`GET /api/v1/users/{id}/stats`
  - 响应：用户统计信息

## 性能优化

- **内存存储**：使用内存存储数据，提高访问速度
- **分页查询**：限制单次查询的返回数量，避免一次返回过多数据
- **缓存**：对高频查询（如用户统计接口）可通过简单缓存减少重复计算
- **线程安全**：使用互斥锁保证多线程环境下的数据安全

## 扩展计划

- **数据库支持**：扩展为使用真实数据库（如MySQL、PostgreSQL）
- **缓存系统**：集成Redis等缓存系统，提高性能
- **负载均衡**：支持多实例部署，实现负载均衡
- **监控与告警**：添加监控和告警功能，提高系统可靠性

## 许可证

MIT License