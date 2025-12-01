#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace api_quota {
namespace http {

class HttpResponse {
public:
    // HTTP状态码
    enum StatusCode {
        OK = 200,
        CREATED = 201,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        INTERNAL_SERVER_ERROR = 500
    };
    
    HttpResponse(StatusCode status_code = OK);
    virtual ~HttpResponse() = default;
    
    // 设置状态码
    void set_status_code(StatusCode status_code);
    
    // 获取状态码
    StatusCode get_status_code() const;
    
    // 获取状态码描述
    std::string get_status_message() const;
    
    // 设置响应头
    void set_header(const std::string& name, const std::string& value);
    
    // 获取响应头
    std::string get_header(const std::string& name) const;
    
    // 获取所有响应头
    std::unordered_map<std::string, std::string> get_headers() const;
    
    // 设置响应体
    void set_body(const std::string& body);
    
    // 获取响应体
    std::string get_body() const;
    
    // 设置JSON响应
    void set_json_body(const std::string& json);
    
    // 生成完整的HTTP响应字符串
    std::string to_string() const;
    
    // 创建成功响应
    static HttpResponse create_success(const std::string& body = "");
    
    // 创建JSON成功响应
    static HttpResponse create_json_success(const std::string& json);
    
    // 创建错误响应
    static HttpResponse create_error(StatusCode status_code, const std::string& message);
    
    // 创建JSON错误响应
    static HttpResponse create_json_error(StatusCode status_code, const std::string& error_code, const std::string& message);
    
private:
    StatusCode status_code_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    
    // 获取状态码对应的消息
    std::string get_status_code_message(StatusCode code) const;
};

} // namespace http
} // namespace api_quota

#endif // HTTP_RESPONSE_HPP