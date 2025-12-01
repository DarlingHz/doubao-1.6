#include "http/http_response.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace api_quota {
namespace http {

HttpResponse::HttpResponse(StatusCode status_code) : status_code_(status_code) {
    // 设置默认Content-Type
    set_header("Content-Type", "text/plain");
    set_header("Server", "API-Quota-Server");
}

void HttpResponse::set_status_code(StatusCode status_code) {
    status_code_ = status_code;
}

HttpResponse::StatusCode HttpResponse::get_status_code() const {
    return status_code_;
}

std::string HttpResponse::get_status_message() const {
    return get_status_code_message(status_code_);
}

void HttpResponse::set_header(const std::string& name, const std::string& value) {
    // 标准化头部名称（首字母大写）
    std::string normalized_name = name;
    bool capitalize_next = true;
    for (char& c : normalized_name) {
        if (std::isspace(c)) {
            capitalize_next = true;
        } else {
            if (capitalize_next) {
                c = std::toupper(c);
                capitalize_next = false;
            } else {
                c = std::tolower(c);
            }
        }
    }
    
    headers_[normalized_name] = value;
}

std::string HttpResponse::get_header(const std::string& name) const {
    // 标准化头部名称进行查找
    std::string normalized_name = name;
    bool capitalize_next = true;
    for (char& c : normalized_name) {
        if (std::isspace(c)) {
            capitalize_next = true;
        } else {
            if (capitalize_next) {
                c = std::toupper(c);
                capitalize_next = false;
            } else {
                c = std::tolower(c);
            }
        }
    }
    
    auto it = headers_.find(normalized_name);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::unordered_map<std::string, std::string> HttpResponse::get_headers() const {
    return headers_;
}

void HttpResponse::set_body(const std::string& body) {
    body_ = body;
    // 更新Content-Length
    set_header("Content-Length", std::to_string(body.size()));
}

std::string HttpResponse::get_body() const {
    return body_;
}

void HttpResponse::set_json_body(const std::string& json) {
    body_ = json;
    set_header("Content-Type", "application/json");
    set_header("Content-Length", std::to_string(json.size()));
}

std::string HttpResponse::to_string() const {
    std::stringstream ss;
    
    // 状态行
    ss << "HTTP/1.1 " << static_cast<int>(status_code_) << " " 
       << get_status_message() << "\r\n";
    
    // 响应头
    for (const auto& [name, value] : headers_) {
        ss << name << ": " << value << "\r\n";
    }
    
    // 空行分隔头部和正文
    ss << "\r\n";
    
    // 响应体
    ss << body_;
    
    return ss.str();
}

HttpResponse HttpResponse::create_success(const std::string& body) {
    HttpResponse response(OK);
    response.set_body(body);
    return response;
}

HttpResponse HttpResponse::create_json_success(const std::string& json) {
    HttpResponse response(OK);
    response.set_json_body(json);
    return response;
}

HttpResponse HttpResponse::create_error(StatusCode status_code, const std::string& message) {
    HttpResponse response(status_code);
    response.set_body(message);
    return response;
}

HttpResponse HttpResponse::create_json_error(StatusCode status_code, const std::string& error_code, const std::string& message) {
    HttpResponse response(status_code);
    
    // 构建简单的JSON错误响应
    std::stringstream json_ss;
    json_ss << "{\"error\":{\"code\":\"" << error_code 
            << "\",\"message\":\"" << message << "\"}}";
    
    response.set_json_body(json_ss.str());
    return response;
}

std::string HttpResponse::get_status_code_message(StatusCode code) const {
    switch (code) {
        case OK:
            return "OK";
        case CREATED:
            return "Created";
        case BAD_REQUEST:
            return "Bad Request";
        case UNAUTHORIZED:
            return "Unauthorized";
        case FORBIDDEN:
            return "Forbidden";
        case NOT_FOUND:
            return "Not Found";
        case INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        default:
            return "Unknown Status";
    }
}

} // namespace http
} // namespace api_quota