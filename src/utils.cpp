#include "utils.h"
#include <regex>
#include <algorithm>
#include <sstream>

std::map<std::string, std::string> parseJson(const std::string& json) {
    std::map<std::string, std::string> result;
    
    // 简单的 JSON 解析（仅支持基本的键值对）
    std::regex keyValuePattern("\"([^\"]+)\"\\s*:\\s*(?:\"([^\"]*)\"|([0-9.]+)|true|false)");
    std::smatch match;
    
    std::string::const_iterator searchStart(json.cbegin());
    while (std::regex_search(searchStart, json.cend(), match, keyValuePattern)) {
        std::string key = match[1];
        std::string value;
        
        if (match[2].matched) {
            value = match[2];
        } else if (match[3].matched) {
            value = match[3];
        } else if (match.str().find("true") != std::string::npos) {
            value = "true";
        } else if (match.str().find("false") != std::string::npos) {
            value = "false";
        }
        
        result[key] = value;
        searchStart = match.suffix().first;
    }
    
    return result;
}

std::vector<std::map<std::string, std::string>> parseJsonArray(const std::string& json) {
    std::vector<std::map<std::string, std::string>> result;
    
    // 查找数组的开始和结束
    size_t start = json.find('[');
    size_t end = json.rfind(']');
    
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        return result;
    }
    
    std::string arrayContent = json.substr(start + 1, end - start - 1);
    
    // 简单分割数组元素（假设元素之间用逗号分隔）
    size_t pos = 0;
    int braceCount = 0;
    size_t elementStart = 0;
    
    while (pos < arrayContent.size()) {
        if (arrayContent[pos] == '{') {
            braceCount++;
        } else if (arrayContent[pos] == '}') {
            braceCount--;
            
            if (braceCount == 0) {
                // 找到一个完整的对象
                std::string element = arrayContent.substr(elementStart, pos - elementStart + 1);
                result.push_back(parseJson(element));
                elementStart = pos + 2; // 跳过逗号和空格
            }
        }
        pos++;
    }
    
    return result;
}

std::string escapeJsonString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING:
            return "PENDING";
        case OrderStatus::PAID:
            return "PAID";
        case OrderStatus::CANCELLED:
            return "CANCELLED";
        case OrderStatus::SHIPPED:
            return "SHIPPED";
        default:
            return "UNKNOWN";
    }
}

std::optional<OrderStatus> stringToOrderStatus(const std::string& statusStr) {
    std::string upperStr = statusStr;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    
    if (upperStr == "PENDING") {
        return OrderStatus::PENDING;
    } else if (upperStr == "PAID") {
        return OrderStatus::PAID;
    } else if (upperStr == "CANCELLED") {
        return OrderStatus::CANCELLED;
    } else if (upperStr == "SHIPPED") {
        return OrderStatus::SHIPPED;
    } else {
        return std::nullopt;
    }
}

std::string productToJson(const Product& product) {
    std::stringstream json;
    json << "{";
    json << "\"id\":" << product.id << ",";
    json << "\"name\":\"" << escapeJsonString(product.name) << "\",";
    json << "\"sku\":\"" << escapeJsonString(product.sku) << "\",";
    json << "\"price\":" << product.price << ",";
    json << "\"stock\":" << product.stock << ",";
    json << "\"reorder_threshold\":" << product.reorder_threshold << ",";
    json << "\"created_at\":\"" << escapeJsonString(product.created_at) << "\",";
    json << "\"updated_at\":\"" << escapeJsonString(product.updated_at) << "\"";
    json << "}";
    return json.str();
}

std::string orderToJson(const Order& order) {
    std::stringstream json;
    json << "{";
    json << "\"id\":" << order.id << ",";
    json << "\"status\":\"" << orderStatusToString(order.status) << "\",";
    json << "\"total_amount\":" << order.total_amount << ",";
    json << "\"created_at\":\"" << escapeJsonString(order.created_at) << "\",";
    json << "\"updated_at\":\"" << escapeJsonString(order.updated_at) << "\",";
    
    if (!order.items.empty()) {
        json << "\"items\":[";
        bool first = true;
        for (const auto& item : order.items) {
            if (!first) json << ",";
            json << "{";
            json << "\"id\":" << item.id << ",";
            json << "\"order_id\":" << item.order_id << ",";
            json << "\"product_id\":" << item.product_id << ",";
            json << "\"quantity\":" << item.quantity << ",";
            json << "\"unit_price\":" << item.unit_price << ",";
            json << "\"subtotal\":" << item.subtotal;
            json << "}";
            first = false;
        }
        json << "]";
    } else {
        json << "\"items\":[]";
    }
    
    json << "}";
    return json.str();
}

// 自定义异常类实现
InvalidJsonException::InvalidJsonException(const std::string& message) 
    : std::runtime_error(message) {
}

ValidationException::ValidationException(const std::string& message) 
    : std::runtime_error(message) {
}
