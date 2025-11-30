# 团队任务与日程管理后端服务

## 项目简介与整体架构

本项目是一个「团队任务与日程管理」后端服务，提供HTTP/JSON接口，支持团队协作中的项目管理、任务追踪、用户认证与权限控制等功能。服务基于C++17开发，采用模块化设计，支持高并发访问和良好的性能表现。

### 核心技术栈

- **C++17**：核心开发语言
- **CMake**：构建系统
- **SQLite**：轻量级数据库，用于数据持久化
- **Socket API**：实现HTTP服务器
- **标准库**：使用C++标准库进行内存管理、线程处理等

### 主要模块划分

- **数据模型层**：定义User、Project、Task等核心实体
- **数据库访问层**：负责与SQLite交互，实现数据持久化
- **认证与授权层**：处理用户注册、登录和权限验证
- **HTTP服务器层**：提供RESTful API接口
- **控制器层**：处理业务逻辑，包括用户、项目、任务和统计功能

## 环境依赖与构建步骤

### 环境要求

- C++17兼容的编译器（如GCC 7+、Clang 5+、MSVC 2019+）
- CMake 3.10或更高版本
- SQLite3库

### 构建步骤

1. **克隆仓库**（如果适用）

```bash
# 假设已经在项目目录中
```

2. **初始化数据库**

```bash
# 执行建表脚本
sqlite3 team_management.db < db/schema.sql
```

3. **构建项目**

```bash
# 创建并进入构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译项目
cmake --build .
```

4. **运行服务**

```bash
# 在构建目录中运行
./team_management_server
```

服务启动后将监听8080端口，所有API接口以`/api/v1`为前缀。

## API文档

### 用户相关

#### POST /api/v1/users/register
- **描述**：用户注册
- **请求体**：
```json
{
  "name": "张三",
  "email": "zhangsan@example.com",
  "password": "password123"
}
```
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "name": "张三",
    "email": "zhangsan@example.com",
    "access_token": "random_token_string",
    "created_at": "2023-01-01 00:00:00"
  }
}
```

#### POST /api/v1/users/login
- **描述**：用户登录
- **请求体**：
```json
{
  "email": "zhangsan@example.com",
  "password": "password123"
}
```
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "name": "张三",
    "email": "zhangsan@example.com",
    "access_token": "random_token_string",
    "created_at": "2023-01-01 00:00:00"
  }
}
```

### 项目管理

#### POST /api/v1/projects
- **描述**：创建项目
- **请求头**：Authorization: Bearer <access_token>
- **请求体**：
```json
{
  "name": "开发项目A",
  "description": "这是一个测试项目"
}
```
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "owner_user_id": 1,
    "name": "开发项目A",
    "description": "这是一个测试项目",
    "created_at": "2023-01-01 00:00:00"
  }
}
```

#### GET /api/v1/projects
- **描述**：获取用户项目列表
- **请求头**：Authorization: Bearer <access_token>
- **查询参数**：page（页码）、page_size（每页大小）
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "projects": [
      {
        "id": 1,
        "owner_user_id": 1,
        "name": "开发项目A",
        "description": "这是一个测试项目",
        "created_at": "2023-01-01 00:00:00"
      }
    ]
  }
}
```

#### GET /api/v1/projects/{project_id}
- **描述**：获取项目详情及统计信息
- **请求头**：Authorization: Bearer <access_token>
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "owner_user_id": 1,
    "name": "开发项目A",
    "description": "这是一个测试项目",
    "created_at": "2023-01-01 00:00:00",
    "stats": {
      "total_tasks": 5,
      "todo_tasks": 2,
      "doing_tasks": 2,
      "done_tasks": 1
    }
  }
}
```

### 任务管理

#### POST /api/v1/projects/{project_id}/tasks
- **描述**：在指定项目下创建任务
- **请求头**：Authorization: Bearer <access_token>
- **请求体**：
```json
{
  "title": "实现登录功能",
  "description": "用户登录验证与Token生成",
  "status": "todo",
  "priority": "high",
  "assignee_user_id": 1,
  "due_date": "2023-01-10"
}
```
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "project_id": 1,
    "title": "实现登录功能",
    "description": "用户登录验证与Token生成",
    "status": "todo",
    "priority": "high",
    "assignee_user_id": 1,
    "created_at": "2023-01-01 00:00:00",
    "updated_at": "2023-01-01 00:00:00"
  }
}
```

#### GET /api/v1/projects/{project_id}/tasks
- **描述**：获取项目任务列表
- **请求头**：Authorization: Bearer <access_token>
- **查询参数**：status、priority、assignee_user_id、due_before、due_after、page、page_size
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "tasks": [
      {
        "id": 1,
        "project_id": 1,
        "title": "实现登录功能",
        "status": "todo",
        "priority": "high",
        "assignee_user_id": 1,
        "created_at": "2023-01-01 00:00:00"
      }
    ]
  }
}
```

#### GET /api/v1/tasks/{task_id}
- **描述**：获取任务详情
- **请求头**：Authorization: Bearer <access_token>
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "project_id": 1,
    "title": "实现登录功能",
    "description": "用户登录验证与Token生成",
    "status": "todo",
    "priority": "high",
    "assignee_user_id": 1,
    "created_at": "2023-01-01 00:00:00",
    "updated_at": "2023-01-01 00:00:00",
    "tags": [
      {"id": 1, "name": "feature"},
      {"id": 2, "name": "urgent"}
    ]
  }
}
```

#### PATCH /api/v1/tasks/{task_id}
- **描述**：更新任务
- **请求头**：Authorization: Bearer <access_token>
- **请求体**：
```json
{
  "status": "doing",
  "description": "用户登录验证与Token生成，添加记住我功能"
}
```
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "id": 1,
    "project_id": 1,
    "title": "实现登录功能",
    "description": "用户登录验证与Token生成，添加记住我功能",
    "status": "doing",
    "priority": "high",
    "assignee_user_id": 1,
    "created_at": "2023-01-01 00:00:00",
    "updated_at": "2023-01-02 00:00:00"
  }
}
```

#### GET /api/v1/tasks/search
- **描述**：搜索任务
- **请求头**：Authorization: Bearer <access_token>
- **查询参数**：keyword、status、tag、due_before、due_after、page、page_size
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "tasks": [
      {
        "id": 1,
        "project_id": 1,
        "title": "实现登录功能",
        "status": "doing",
        "priority": "high",
        "assignee_user_id": 1,
        "created_at": "2023-01-01 00:00:00"
      }
    ]
  }
}
```

### 统计与审计日志

#### GET /api/v1/stats/overview
- **描述**：获取用户统计概览
- **请求头**：Authorization: Bearer <access_token>
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "task_stats": {
      "todo": 5,
      "doing": 3,
      "done": 2,
      "total": 10
    },
    "overdue_tasks": 1,
    "recent_created_tasks": 8,
    "projects_count": 3
  }
}
```

#### GET /api/v1/audit_logs
- **描述**：获取审计日志
- **请求头**：Authorization: Bearer <access_token>
- **查询参数**：limit（日志数量限制）
- **响应**：
```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "audit_logs": [
      {
        "id": 1,
        "user_id": 1,
        "action_type": "create_task",
        "resource_type": "task",
        "resource_id": 1,
        "created_at": "2023-01-01 00:00:00",
        "detail": "Task created: 实现登录功能"
      }
    ]
  }
}
```

## 性能与优化说明

本项目在设计时考虑了高并发和资源使用效率，主要实现了以下优化措施：

### 1. 数据库连接池

- **实现方式**：创建固定数量的数据库连接，存储在连接池中
- **优化原理**：避免每次请求都重新建立和关闭数据库连接，减少资源消耗
- **配置**：可在主程序中调整连接池大小（默认5个连接）
- **使用场景**：所有数据库操作都会复用连接池中的连接，特别是高频请求如任务查询

### 2. 高效索引设计

- **实现方式**：在数据库表中为常用查询字段建立索引
- **主要索引**：
  - `tasks`表：status、priority、project_id、assignee_user_id、due_date
  - `projects`表：owner_user_id
  - `users`表：email（唯一索引）
- **优化原理**：加速查询操作，特别是在大量数据情况下，显著减少查询时间
- **使用场景**：任务过滤、项目列表获取、用户登录验证等高频操作

### 3. 内存缓存策略

- **实现方式**：在AuthService中使用内存映射存储用户Token
- **优化原理**：避免每次请求都查询数据库验证Token，减少数据库负载
- **使用场景**：所有需要认证的API请求，特别是高频的任务操作请求

### 4. 异步处理设计

- **实现方式**：使用多线程处理HTTP请求
- **优化原理**：允许并发处理多个客户端请求，提高服务器吞吐量
- **使用场景**：所有API请求处理

## 假设与限制

### 业务假设

1. **用户权限模型**：当前实现的权限控制相对简单，仅判断用户是否为项目所有者或成员
2. **任务状态流转**：实现了单向流转规则（TODO -> DOING -> DONE），不允许逆向流转
3. **密码安全**：当前使用简单的哈希函数，不适用于生产环境
4. **Token有效期**：当前实现中Token没有过期机制，在实际生产环境中应考虑添加

### 技术限制

1. **数据库选择**：使用SQLite作为轻量级数据库，适合开发和小规模部署，但在高并发场景下可能存在性能瓶颈
2. **HTTP服务器**：当前实现的HTTP服务器较为简单，可能在极端负载下存在性能问题
3. **缓存机制**：当前缓存仅在单进程内存中，在多进程或分布式部署时需要考虑共享缓存
4. **错误处理**：当前错误处理较为基础，在复杂场景下可能需要更完善的错误恢复机制

## 可能的进一步优化方向

1. 使用更安全的密码哈希算法（如bcrypt、Argon2）
2. 实现Token过期和刷新机制
3. 添加数据压缩和加密传输
4. 引入分布式缓存（如Redis）
5. 支持数据库读写分离
6. 实现更完善的监控和性能分析
7. 优化数据库查询，使用预编译语句减少SQL注入风险
