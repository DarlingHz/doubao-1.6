# Personal Knowledge Cards 个人知识卡片系统

一个使用C++实现的个人知识卡片管理系统，支持卡片的创建、编辑、搜索、标签管理等功能。

## 功能特性

- **用户认证**：注册、登录、JWT令牌验证
- **卡片管理**：创建、查看、更新、删除知识卡片
- **标签系统**：添加标签、按标签筛选、重命名标签、合并标签
- **搜索功能**：按关键词搜索卡片
- **统计分析**：卡片数量统计、标签使用频率、最近创建卡片统计
- **数据持久化**：使用MySQL存储数据

## 技术栈

- **语言**：C++17
- **Web框架**：httplib (C++ HTTP库)
- **数据库**：MySQL
- **JSON处理**：nlohmann/json
- **加密库**：OpenSSL (用于密码加密和JWT)
- **构建系统**：CMake
- **测试框架**：Google Test

## 系统要求

- C++17兼容的编译器 (GCC 7+, Clang 5+, MSVC 2019+)
- CMake 3.14或更高版本
- MySQL 5.7或更高版本
- OpenSSL库
- nlohmann/json库
- httplib库
- Google Test (用于测试)

## 安装说明

### 1. 安装依赖

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y g++ cmake libmysqlclient-dev libssl-dev libgtest-dev
# 安装nlohmann/json
sudo apt-get install -y nlohmann-json3-dev
# 安装httplib (可能需要从源码安装)
git clone https://github.com/yhirose/cpp-httplib.git
cd cpp-httplib
cmake -B build
cmake --build build
cmake --install build
```

#### macOS
```bash
brew install cmake mysql openssl googletest nlohmann-json
# 安装httplib
git clone https://github.com/yhirose/cpp-httplib.git
cd cpp-httplib
cmake -B build
cmake --build build
cmake --install build
```

### 2. 克隆项目

```bash
git clone [项目仓库地址]
cd PersonalKnowledgeCards
```

### 3. 创建数据库

```sql
CREATE DATABASE knowledge_cards;
USE knowledge_cards;

-- 创建用户表
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- 创建卡片表
CREATE TABLE cards (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    title VARCHAR(255) NOT NULL,
    content TEXT,
    is_pinned BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 创建标签表
CREATE TABLE tags (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_user_tag (user_id, name),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 创建卡片-标签关联表
CREATE TABLE card_tags (
    card_id INT NOT NULL,
    tag_id INT NOT NULL,
    PRIMARY KEY (card_id, tag_id),
    FOREIGN KEY (card_id) REFERENCES cards(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);
```

### 4. 配置数据库连接

修改 `src/main.cpp` 中的数据库配置：

```cpp
db::DatabaseConfig config;
config.host = "localhost";
config.port = 3306;
config.database = "knowledge_cards";
config.username = "root";  // 你的MySQL用户名
config.password = "password";  // 你的MySQL密码
config.poolSize = 10;
```

### 5. 构建项目

```bash
mkdir build
cd build
cmake ..
make
```

## 运行项目

```bash
./src/server
```

服务器将在端口 8080 上启动。你可以通过 http://localhost:8080/health 检查服务器是否正常运行。

## API文档

### 认证接口

- **POST /api/register** - 用户注册
  - 请求体: `{"email": "user@example.com", "password": "password123"}`
  - 响应: `{"code": 0, "message": "注册成功", "data": {"token": "...", "userId": 1, "email": "..."}}`

- **POST /api/login** - 用户登录
  - 请求体: `{"email": "user@example.com", "password": "password123"}`
  - 响应: `{"code": 0, "message": "登录成功", "data": {"token": "...", "userId": 1, "email": "..."}}`

### 卡片接口

- **POST /api/cards** - 创建卡片
  - 请求头: `Authorization: Bearer <token>`
  - 请求体: `{"title": "卡片标题", "content": "卡片内容", "tags": ["标签1", "标签2"], "is_pinned": false}`
  - 响应: `{"code": 0, "message": "创建成功", "data": {...}}`

- **GET /api/cards** - 获取卡片列表
  - 请求头: `Authorization: Bearer <token>`
  - 查询参数: `?page=1&page_size=10&sort_by=created_at&sort_order=desc`
  - 响应: `{"code": 0, "message": "获取成功", "data": {"total": 100, "page": 1, "page_size": 10, "items": [...]}}`

- **GET /api/cards/:id** - 获取单个卡片
  - 请求头: `Authorization: Bearer <token>`
  - 响应: `{"code": 0, "message": "获取成功", "data": {...}}`

- **PUT /api/cards/:id** - 更新卡片
  - 请求头: `Authorization: Bearer <token>`
  - 请求体: `{"title": "新标题", "content": "新内容", "tags": ["标签1"], "is_pinned": true}`
  - 响应: `{"code": 0, "message": "更新成功", "data": {...}}`

- **DELETE /api/cards/:id** - 删除卡片
  - 请求头: `Authorization: Bearer <token>`
  - 响应: `{"code": 0, "message": "删除成功"}`

- **GET /api/cards/search** - 搜索卡片
  - 请求头: `Authorization: Bearer <token>`
  - 查询参数: `?keyword=关键词&page=1&page_size=10`
  - 响应: `{"code": 0, "message": "搜索成功", "data": {...}}`

### 标签接口

- **GET /api/tags** - 获取所有标签
  - 请求头: `Authorization: Bearer <token>`
  - 响应: `{"code": 0, "message": "获取成功", "data": [{"id": 1, "name": "标签名", "card_count": 5}]}`

- **POST /api/tags/rename** - 重命名标签
  - 请求头: `Authorization: Bearer <token>`
  - 请求体: `{"old_name": "旧标签名", "new_name": "新标签名"}`
  - 响应: `{"code": 0, "message": "重命名成功"}`

- **POST /api/tags/merge** - 合并标签
  - 请求头: `Authorization: Bearer <token>`
  - 请求体: `{"tag_to_merge": "要合并的标签", "target_tag": "目标标签"}`
  - 响应: `{"code": 0, "message": "合并成功"}`

- **GET /api/tags/top** - 获取热门标签
  - 请求头: `Authorization: Bearer <token>`
  - 查询参数: `?limit=10`
  - 响应: `{"code": 0, "message": "获取成功", "data": [...]}`

### 统计接口

- **GET /api/statistics** - 获取统计信息
  - 请求头: `Authorization: Bearer <token>`
  - 响应: `{"code": 0, "message": "获取成功", "data": {"total_cards": 100, "top_tags": [...], "recent_daily_counts": [...]}}`

## 运行测试

```bash
cd build
make run_tests
```

## 项目结构

```
├── CMakeLists.txt          # 主CMake构建文件
├── README.md               # 项目说明文档
├── src/                    # 源代码目录
│   ├── CMakeLists.txt      # 源码目录CMake构建文件
│   ├── controller/         # 控制器层
│   ├── dao/                # 数据访问层实现
│   ├── db/                 # 数据库相关实现
│   ├── include/            # 头文件
│   │   ├── dao/            # 数据访问层接口
│   │   ├── db/             # 数据库相关接口
│   │   ├── model/          # 数据模型
│   │   └── service/        # 服务层接口
│   ├── main.cpp            # 主程序入口
│   └── service/            # 服务层实现
└── tests/                  # 测试目录
    ├── CMakeLists.txt      # 测试目录CMake构建文件
    └── UserServiceTest.cpp # 示例测试文件
```

## 注意事项

- 本项目使用JWT进行身份认证，请妥善保管密钥
- 在生产环境中，请勿在代码中硬编码数据库密码，建议使用环境变量或配置文件
- 定期备份数据库以防止数据丢失
- 考虑添加更多的安全措施，如请求限流、输入验证等

## 许可证

[MIT License](LICENSE)
