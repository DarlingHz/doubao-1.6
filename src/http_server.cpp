#include "http_server.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>

// JsonValue类实现
JsonValue::JsonValue() : type_(JsonType::Null) {}

JsonValue::JsonValue(const std::string& str) : type_(JsonType::String), string_value_(str) {}

JsonValue::JsonValue(int num) : type_(JsonType::Number), number_value_(num) {}

JsonValue::JsonValue(bool b) : type_(JsonType::Boolean), bool_value_(b) {}

JsonValue::JsonValue(JsonType type) : type_(type) {
    if (type == JsonType::Object) {
        object_value_ = std::map<std::string, JsonValue>();
    } else if (type == JsonType::Array) {
        array_value_ = std::vector<JsonValue>();
    }
}

void JsonValue::add(const std::string& key, const JsonValue& value) {
    if (type_ != JsonType::Object) {
        type_ = JsonType::Object;
        object_value_.clear();
    }
    object_value_[key] = value;
}

JsonValue JsonValue::get(const std::string& key) const {
    if (type_ == JsonType::Object) {
        auto it = object_value_.find(key);
        if (it != object_value_.end()) {
            return it->second;
        }
    }
    return JsonValue(); // 返回null值
}

void JsonValue::push(const JsonValue& value) {
    if (type_ != JsonType::Array) {
        type_ = JsonType::Array;
        array_value_.clear();
    }
    array_value_.push_back(value);
}

JsonValue JsonValue::at(size_t index) const {
    if (type_ == JsonType::Array && index < array_value_.size()) {
        return array_value_[index];
    }
    return JsonValue(); // 返回null值
}

size_t JsonValue::size() const {
    if (type_ == JsonType::Array) {
        return array_value_.size();
    }
    return 0;
}

std::string JsonValue::toString() const {
    switch (type_) {
        case JsonType::String:
            return string_value_;
        case JsonType::Number:
            return std::to_string(number_value_);
        case JsonType::Boolean:
            return bool_value_ ? "true" : "false";
        case JsonType::Object:
        case JsonType::Array:
        case JsonType::Null:
        default:
            return "";
    }
}

int JsonValue::toInt() const {
    if (type_ == JsonType::Number) {
        return number_value_;
    }
    return 0;
}

bool JsonValue::toBool() const {
    if (type_ == JsonType::Boolean) {
        return bool_value_;
    }
    return false;
}

// 简单的JSON解析实现
JsonValue JsonValue::parse(const std::string& json_str) {
    // 这里实现一个非常简单的JSON解析器，仅支持基本类型
    // 实际应用中应该使用更完善的解析器
    if (json_str.empty()) {
        return JsonValue();
    }
    
    // 处理字符串
    if (json_str[0] == '"' && json_str.back() == '"') {
        return JsonValue(json_str.substr(1, json_str.size() - 2));
    }
    
    // 处理数字
    try {
        size_t pos;
        int num = std::stoi(json_str, &pos);
        if (pos == json_str.size()) {
            return JsonValue(num);
        }
    } catch (...) {
    }
    
    // 处理布尔值
    if (json_str == "true") {
        return JsonValue(true);
    }
    if (json_str == "false") {
        return JsonValue(false);
    }
    
    // 处理null
    if (json_str == "null") {
        return JsonValue();
    }
    
    // 处理对象和数组（简化版）
    if (json_str[0] == '{') {
        JsonValue obj(JsonType::Object);
        // 这里应该解析对象内容
        return obj;
    }
    
    if (json_str[0] == '[') {
        JsonValue arr(JsonType::Array);
        // 这里应该解析数组内容
        return arr;
    }
    
    return JsonValue();
}

// HTTP请求解析
HttpRequest parseHttpRequest(const std::string& request_str) {
    HttpRequest request;
    std::istringstream stream(request_str);
    std::string line;
    
    // 解析请求行
    if (std::getline(stream, line)) {
        if (line.back() == '\r') {
            line.pop_back();
        }
        std::istringstream request_line(line);
        request_line >> request.method >> request.path >> request.http_version;
        
        // 解析查询参数
        size_t query_pos = request.path.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = request.path.substr(query_pos + 1);
            request.path = request.path.substr(0, query_pos); // 移除查询参数部分
            
            // 解析查询参数
            std::istringstream query_stream(query_string);
            std::string param;
            while (std::getline(query_stream, param, '&')) {
                size_t equals_pos = param.find('=');
                if (equals_pos != std::string::npos) {
                    std::string key = param.substr(0, equals_pos);
                    std::string value = param.substr(equals_pos + 1);
                    request.query_params[key] = value;
                }
            }
        }
    }
    
    // 解析头部
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') {
            line.pop_back();
        }
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // 跳过冒号和空格
            request.headers[key] = value;
        }
    }
    
    // 解析请求体
    std::string body;
    while (std::getline(stream, line)) {
        if (!body.empty()) {
            body += "\n";
        }
        body += line;
    }
    // 使用JsonValue::parse解析请求体
    if (!body.empty()) {
        try {
            request.body = JsonValue::parse(body);
        } catch (...) {
            // 如果解析失败，创建一个空的JSON对象
            request.body = JsonValue(JsonType::Object);
        }
    } else {
        // 空请求体，创建一个空的JSON对象
        request.body = JsonValue(JsonType::Object);
    }
    
    return request;
}

// HTTP响应生成
std::string generateHttpResponse(const HttpResponse& response) {
    std::stringstream ss;
    
    // 状态行
    ss << "HTTP/1.1 " << response.status_code << " " << response.status_message << "\r\n";
    
    // 头部
    for (const auto& [key, value] : response.headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    // 确保Content-Type头存在
    if (response.headers.find("Content-Type") == response.headers.end()) {
        ss << "Content-Type: application/json\r\n";
    }
    
    // 空行分隔头部和正文
    ss << "\r\n";
    
    // 正文 - 直接使用字符串响应体
    ss << response.body;
    
    return ss.str();
}

// HttpServer 实现
HttpServer::HttpServer(int port, std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service) 
    : port_(port), db_(db), auth_service_(auth_service), running_(false) {
}

void HttpServer::start() {
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // 绑定地址和端口
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 开始监听
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
    
    // 处理连接的主循环
    running_ = true;
    while (running_) {
        int new_socket;
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket >= 0) {
            // 简单的单线程处理
            char buffer[1024] = {0};
            std::string request_str;
            int bytes_read;
            
            while ((bytes_read = read(new_socket, buffer, 1024)) > 0) {
                request_str.append(buffer, bytes_read);
                if (request_str.find("\r\n\r\n") != std::string::npos) {
                    break;
                }
            }
            
            if (bytes_read > 0) {
                HttpRequest request = parseHttpRequest(request_str);
                HttpResponse response = handleRequest(request);
                std::string response_str = generateHttpResponse(response);
                send(new_socket, response_str.c_str(), response_str.size(), 0);
            }
            
            close(new_socket);
        }
    }
    
    close(server_fd);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::registerRoute(const std::string& method, const std::string& path, const RouteHandler& handler) {
    routes_[method + " " + path] = handler;
}

HttpResponse HttpServer::handleRequest(const HttpRequest& request) {
    // 尝试精确匹配
    auto route_it = routes_.find(request.method + " " + request.path);
    if (route_it != routes_.end()) {
        try {
            return route_it->second(request);
        } catch (const std::exception& e) {
            HttpResponse response;
                response.status_code = 500;
                response.status_message = "Internal Server Error";
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"error\": \"Internal Server Error\"}";
                return response;
        }
    }
    
    // 路径不匹配
    HttpResponse response;
    response.status_code = 404;
    response.status_message = "Not Found";
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"error\": \"Not Found\"}";
    return response;
}

std::string HttpServer::extractPathParam(const std::string& path_pattern, const std::string& actual_path, const std::string& param_name) {
    // 简单实现
    return "";
}

bool HttpServer::matchPath(const std::string& pattern, const std::string& path) {
    return pattern == path;
}

// Controller 实现

HttpResponse Controller::successResponse(const JsonValue& data) {
    HttpResponse response;
    response.status_code = 200;
    response.status_message = "OK";
    response.headers["Content-Type"] = "application/json";
    
    // 直接构建JSON字符串响应
    response.body = "{\"code\": 200, \"message\": \"success\", \"data\": " + data.toString() + "}";
    
    return response;
}

HttpResponse Controller::errorResponse(int code, const std::string& message) {
    HttpResponse response;
    response.status_code = code;
    
    // 设置状态消息
    switch (code) {
        case 400: response.status_message = "Bad Request";
            break;
        case 401: response.status_message = "Unauthorized";
            break;
        case 403: response.status_message = "Forbidden";
            break;
        case 404: response.status_message = "Not Found";
            break;
        case 500: response.status_message = "Internal Server Error";
            break;
        default: response.status_message = "Unknown Error";
    }
    
    response.headers["Content-Type"] = "application/json";
    
    // 直接构建JSON字符串响应
    response.body = "{\"code\": " + std::to_string(code) + ", \"message\": \"" + message + "\", \"data\": null}";
    
    return response;
}

std::optional<User> Controller::authenticateRequest(const HttpRequest& request) {
    // 从请求头获取认证令牌
    auto it = request.headers.find("Authorization");
    if (it != request.headers.end()) {
        std::string auth_header = it->second;
        // 假设格式为 "Bearer <token>"
        if (auth_header.substr(0, 7) == "Bearer ") {
            std::string token = auth_header.substr(7);
            return auth_service_->verifyToken(token);
        }
    }
    return std::nullopt;
}

bool Controller::parseRequestBody(const HttpRequest& request, JsonValue& body) {
    // 使用我们的自定义JsonValue解析请求体
    try {
        // 由于request.body已经是JsonValue类型，直接赋值
        body = request.body;
        return true;
    } catch (...) {
        return false;
    }
}

// 删除不需要的辅助函数，使用自定义JsonValue类代替