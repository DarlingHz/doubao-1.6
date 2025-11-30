#include "error_code.h"

ErrorCodeManager& ErrorCodeManager::Instance() {
    static ErrorCodeManager instance;
    return instance;
}

void ErrorCodeManager::Init() {
    if (is_initialized_) {
        return;
    }

    // 初始化错误信息映射
    error_messages_[ErrorCode::SUCCESS] = "Success";
    error_messages_[ErrorCode::INVALID_PARAM] = "Invalid parameter";
    error_messages_[ErrorCode::USER_NOT_FOUND] = "User not found";
    error_messages_[ErrorCode::WRONG_PASSWORD] = "Wrong password";
    error_messages_[ErrorCode::USERNAME_EXISTS] = "Username already exists";
    error_messages_[ErrorCode::NOT_LOGGED_IN] = "Not logged in";
    error_messages_[ErrorCode::INVALID_TOKEN] = "Invalid token";
    error_messages_[ErrorCode::TOKEN_EXPIRED] = "Token expired";
    error_messages_[ErrorCode::PERMISSION_DENIED] = "Permission denied";
    error_messages_[ErrorCode::RESOURCE_NOT_FOUND] = "Resource not found";
    error_messages_[ErrorCode::INTERNAL_SERVER_ERROR] = "Internal server error";
    error_messages_[ErrorCode::DATABASE_ERROR] = "Database error";
    error_messages_[ErrorCode::NETWORK_ERROR] = "Network error";
    error_messages_[ErrorCode::FILE_ERROR] = "File error";
    error_messages_[ErrorCode::VALIDATION_ERROR] = "Validation error";
    error_messages_[ErrorCode::LIMIT_EXCEEDED] = "Limit exceeded";

    // 初始化错误代码字符串映射
    error_code_strings_[ErrorCode::SUCCESS] = "SUCCESS";
    error_code_strings_[ErrorCode::INVALID_PARAM] = "INVALID_PARAM";
    error_code_strings_[ErrorCode::USER_NOT_FOUND] = "USER_NOT_FOUND";
    error_code_strings_[ErrorCode::WRONG_PASSWORD] = "WRONG_PASSWORD";
    error_code_strings_[ErrorCode::USERNAME_EXISTS] = "USERNAME_EXISTS";
    error_code_strings_[ErrorCode::NOT_LOGGED_IN] = "NOT_LOGGED_IN";
    error_code_strings_[ErrorCode::INVALID_TOKEN] = "INVALID_TOKEN";
    error_code_strings_[ErrorCode::TOKEN_EXPIRED] = "TOKEN_EXPIRED";
    error_code_strings_[ErrorCode::PERMISSION_DENIED] = "PERMISSION_DENIED";
    error_code_strings_[ErrorCode::RESOURCE_NOT_FOUND] = "RESOURCE_NOT_FOUND";
    error_code_strings_[ErrorCode::INTERNAL_SERVER_ERROR] = "INTERNAL_SERVER_ERROR";
    error_code_strings_[ErrorCode::DATABASE_ERROR] = "DATABASE_ERROR";
    error_code_strings_[ErrorCode::NETWORK_ERROR] = "NETWORK_ERROR";
    error_code_strings_[ErrorCode::FILE_ERROR] = "FILE_ERROR";
    error_code_strings_[ErrorCode::VALIDATION_ERROR] = "VALIDATION_ERROR";
    error_code_strings_[ErrorCode::LIMIT_EXCEEDED] = "LIMIT_EXCEEDED";

    is_initialized_ = true;
}

std::string ErrorCodeManager::GetErrorMessage(ErrorCode code) const {
    auto it = error_messages_.find(code);
    if (it != error_messages_.end()) {
        return it->second;
    }
    return "Unknown error";
}

std::string ErrorCodeManager::GetErrorCodeString(ErrorCode code) const {
    auto it = error_code_strings_.find(code);
    if (it != error_code_strings_.end()) {
        return it->second;
    }
    return "UNKNOWN_ERROR";
}
