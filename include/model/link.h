#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace model {

// 短链接状态
enum class LinkStatus {
    ACTIVE,
    DISABLED,
    EXPIRED
};

// 短链接模型
struct ShortLink {
    uint64_t id;  // 唯一标识
    std::string longUrl;  // 原始长链接
    std::string shortCode;  // 短码
    std::string customAlias;  // 自定义别名（如果有）
    uint64_t createdAt;  // 创建时间戳
    uint64_t expireAt;  // 过期时间戳，0表示永不过期
    LinkStatus status;  // 状态
    uint64_t visitCount;  // 访问次数
    
    // 构造函数
    ShortLink() : id(0), createdAt(0), expireAt(0), status(LinkStatus::ACTIVE), visitCount(0) {}
    
    // 检查是否已过期
    bool isExpired() const {
        return expireAt > 0 && expireAt < (uint64_t)time(nullptr);
    }
    
    // 检查是否可访问
    bool isAccessible() const {
        return status == LinkStatus::ACTIVE && !isExpired();
    }
};

// 访问日志模型
struct VisitLog {
    uint64_t id;  // 唯一标识
    uint64_t linkId;  // 关联的短链接ID
    uint64_t visitTime;  // 访问时间戳
    std::string ipAddress;  // 访问者IP地址
    std::string userAgent;  // 用户代理（可选）
    
    // 构造函数
    VisitLog() : id(0), linkId(0), visitTime(0) {}
};

// 短链接统计信息
struct LinkStats {
    ShortLink linkInfo;  // 短链接基本信息
    std::vector<VisitLog> recentVisits;  // 最近的访问日志
    uint64_t totalVisits;  // 总访问次数
};

// 创建短链接请求
struct CreateLinkRequest {
    std::string longUrl;  // 必需
    int expireSeconds;  // 可选，过期时间（秒）
    std::string customAlias;  // 可选，自定义别名
    
    CreateLinkRequest() : expireSeconds(0) {}
};

// 创建短链接响应
struct CreateLinkResponse {
    bool success;  // 是否成功
    std::string message;  // 消息
    std::string shortCode;  // 短码
    std::string fullShortUrl;  // 完整短链接URL
    uint64_t expireAt;  // 过期时间戳
    
    CreateLinkResponse() : success(false) {}
};

// API响应结构
template <typename T>
struct ApiResponse {
    bool success;  // 是否成功
    std::string message;  // 消息
    T data;  // 数据
    
    ApiResponse() : success(false) {}
    
    static ApiResponse<T> successResponse(const T& data, const std::string& message = "Success") {
        ApiResponse<T> response;
        response.success = true;
        response.message = message;
        response.data = data;
        return response;
    }
    
    static ApiResponse<T> errorResponse(const std::string& message) {
        ApiResponse<T> response;
        response.success = false;
        response.message = message;
        return response;
    }
};

} // namespace model
