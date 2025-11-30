#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <string>
#include <unordered_map>

enum class ErrorCode {
    SUCCESS = 0,
    INVALID_PARAM = 1,
    USER_NOT_FOUND = 2,
    WRONG_PASSWORD = 3,
    USERNAME_EXISTS = 4,
    NOT_LOGGED_IN = 5,
    INVALID_TOKEN = 6,
    TOKEN_EXPIRED = 7,
    PERMISSION_DENIED = 8,
    RESOURCE_NOT_FOUND = 9,
    INTERNAL_SERVER_ERROR = 10,
    DATABASE_ERROR = 11,
    NETWORK_ERROR = 12,
    FILE_ERROR = 13,
    VALIDATION_ERROR = 14,
    LIMIT_EXCEEDED = 15
};

class ErrorCodeManager {
public:
    ErrorCodeManager() = default;
    ~ErrorCodeManager() = default;

    // 禁止拷贝和移动
    ErrorCodeManager(const ErrorCodeManager&) = delete;
    ErrorCodeManager& operator=(const ErrorCodeManager&) = delete;
    ErrorCodeManager(ErrorCodeManager&&) = delete;
    ErrorCodeManager& operator=(ErrorCodeManager&&) = delete;

    // 单例模式
    static ErrorCodeManager& Instance();

    // 初始化错误码映射
    void Init();

    // 根据错误码获取错误信息
    std::string GetErrorMessage(ErrorCode code) const;

    // 根据错误码获取错误代码字符串
    std::string GetErrorCodeString(ErrorCode code) const;

private:
    std::unordered_map<ErrorCode, std::string> error_messages_;
    std::unordered_map<ErrorCode, std::string> error_code_strings_;
    bool is_initialized_ = false;
};

#endif // ERROR_CODE_H
