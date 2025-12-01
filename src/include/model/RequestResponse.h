#ifndef REQUEST_RESPONSE_H
#define REQUEST_RESPONSE_H

#include <string>
#include <vector>
#include <map>

namespace model {

// 通用响应结构
template <typename T>
struct ApiResponse {
    int code;          // 状态码：0表示成功，非0表示错误
    std::string message; // 状态消息
    T data;           // 响应数据
};

// 空数据响应
template <>
struct ApiResponse<void> {
    int code;          // 状态码：0表示成功，非0表示错误
    std::string message; // 状态消息
};

// 用户注册请求
truct RegisterRequest {
    std::string email;
    std::string password;
};

// 用户登录请求
truct LoginRequest {
    std::string email;
    std::string password;
};

// 登录响应
truct LoginResponse {
    std::string token; // JWT token
    int userId;        // 用户ID
    std::string email; // 用户邮箱
};

// 创建卡片请求
truct CreateCardRequest {
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    bool is_pinned = false;
};

// 更新卡片请求
truct UpdateCardRequest {
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    bool is_pinned;
};

// 卡片列表查询参数
truct CardListQuery {
    int page = 1;
    int page_size = 20;
    std::string sort_by = "updated_at"; // created_at 或 updated_at
    std::string sort_order = "desc";    // asc 或 desc
    std::vector<std::string> tags;      // 按标签过滤
    std::string keyword;                // 搜索关键字
};

// 分页响应
template <typename T>
struct PaginatedResponse {
    int total;       // 总记录数
    int page;        // 当前页码
    int page_size;   // 每页大小
    std::vector<T> items; // 数据项
};

} // namespace model

#endif // REQUEST_RESPONSE_H