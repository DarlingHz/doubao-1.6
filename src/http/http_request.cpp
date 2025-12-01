#include "http/http_request.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace api_quota {
namespace http {

SimpleHttpRequest::SimpleHttpRequest(const std::string& method,
                                  const std::string& path,
                                  const std::string& body) 
    : method_(method),
      path_(path),
      body_(body) {
    // 解析路径和查询字符串
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path_ = path.substr(0, query_pos);
        std::string query_str = path.substr(query_pos + 1);
        parse_query_string(query_str);
    } else {
        path_ = path;
    }
    
    // 设置默认Content-Type
    set_header("Content-Type", "text/plain");
}

std::unique_ptr<SimpleHttpRequest> SimpleHttpRequest::parse(const std::string& request_str) {
    std::istringstream iss(request_str);
    std::string line;
    
    // 解析请求行
    if (!std::getline(iss, line)) {
        return nullptr;
    }
    
    std::istringstream request_line_iss(line);
    std::string method, path, version;
    request_line_iss >> method >> path >> version;
    
    if (method.empty() || path.empty()) {
        return nullptr;
    }
    
    auto request = std::make_unique<SimpleHttpRequest>(method, path);
    
    // 解析请求头
    std::vector<std::string> header_lines;
    while (std::getline(iss, line) && !line.empty() && line != "\r") {
        if (line.back() == '\r') {
            line.pop_back();
        }
        header_lines.push_back(line);
    }
    request->parse_headers(header_lines);
    
    // 解析请求体
    // 检查Content-Length头
    std::string content_length_str = request->get_header("Content-Length");
    if (!content_length_str.empty()) {
        try {
            size_t content_length = std::stoul(content_length_str);
            std::string body;
            body.resize(content_length);
            iss.read(&body[0], content_length);
            request->body_ = body;
        } catch (...) {
            // 忽略Content-Length解析错误
        }
    }
    
    return request;
}

void SimpleHttpRequest::parse_query_string(const std::string& query_str) {
    std::istringstream iss(query_str);
    std::string param;
    
    while (std::getline(iss, param, '&')) {
        size_t eq_pos = param.find('=');
        if (eq_pos != std::string::npos) {
            std::string name = param.substr(0, eq_pos);
            std::string value = param.substr(eq_pos + 1);
            // 简单的URL解码
            // 注意：在生产环境中应该使用更健壮的URL解码实现
            query_params_[name] = value;
        }
    }
}

void SimpleHttpRequest::parse_headers(const std::vector<std::string>& header_lines) {
    for (const auto& line : header_lines) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            // 去掉冒号后的空格
            size_t value_start = colon_pos + 1;
            while (value_start < line.size() && std::isspace(line[value_start])) {
                value_start++;
            }
            std::string value = line.substr(value_start);
            
            // 标准化头部名称（全部小写）
            std::transform(name.begin(), name.end(), name.begin(), 
                          [](unsigned char c){ return std::tolower(c); });
            
            headers_[name] = value;
        }
    }
}

std::string SimpleHttpRequest::get_method() const {
    return method_;
}

std::string SimpleHttpRequest::get_path() const {
    return path_;
}

std::string SimpleHttpRequest::get_query_param(const std::string& name, const std::string& default_value) const {
    auto it = query_params_.find(name);
    if (it != query_params_.end()) {
        return it->second;
    }
    return default_value;
}

std::unordered_map<std::string, std::string> SimpleHttpRequest::get_query_params() const {
    return query_params_;
}

std::string SimpleHttpRequest::get_path_param(const std::string& name, const std::string& default_value) const {
    auto it = path_params_.find(name);
    if (it != path_params_.end()) {
        return it->second;
    }
    return default_value;
}

std::string SimpleHttpRequest::get_header(const std::string& name, const std::string& default_value) const {
    // 标准化头部名称（全部小写）
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c){ return std::tolower(c); });
    
    auto it = headers_.find(lower_name);
    if (it != headers_.end()) {
        return it->second;
    }
    return default_value;
}

std::unordered_map<std::string, std::string> SimpleHttpRequest::get_headers() const {
    return headers_;
}

std::string SimpleHttpRequest::get_body() const {
    return body_;
}

void SimpleHttpRequest::set_path_param(const std::string& name, const std::string& value) {
    path_params_[name] = value;
}

bool SimpleHttpRequest::is_json_content_type() const {
    std::string content_type = get_header("content-type");
    return content_type.find("application/json") != std::string::npos;
}

void SimpleHttpRequest::set_query_param(const std::string& name, const std::string& value) {
    query_params_[name] = value;
}

void SimpleHttpRequest::set_header(const std::string& name, const std::string& value) {
    // 标准化头部名称（全部小写）
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                  [](unsigned char c){ return std::tolower(c); });
    
    headers_[lower_name] = value;
}

} // namespace http
} // namespace api_quota