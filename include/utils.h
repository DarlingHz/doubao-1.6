#ifndef UTILS_H
#define UTILS_H

#include "models.h"
#include <string>
#include <map>
#include <vector>
#include <optional>

// JSON 解析辅助函数
std::map<std::string, std::string> parseJson(const std::string& json);
std::vector<std::map<std::string, std::string>> parseJsonArray(const std::string& json);
std::string escapeJsonString(const std::string& str);

// 枚举转换函数
std::string orderStatusToString(OrderStatus status);
std::optional<OrderStatus> stringToOrderStatus(const std::string& statusStr);

// 实体转 JSON 函数
std::string productToJson(const Product& product);
std::string orderToJson(const Order& order);

// 自定义异常类
class InvalidJsonException : public std::runtime_error {
public:
    InvalidJsonException(const std::string& message);
};

class ValidationException : public std::runtime_error {
public:
    ValidationException(const std::string& message);
};

#endif // UTILS_H
