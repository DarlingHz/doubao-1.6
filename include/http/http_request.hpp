#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace api_quota {
namespace http {

class HttpRequest {
public:
    virtual ~HttpRequest() = default;
    
    // 获取HTTP方法
    virtual std::string get_method() const = 0;
    
    // 获取请求路径
    virtual std::string get_path() const = 0;
    
    // 获取查询参数
    virtual std::string get_query_param(const std::string& name, const std::string& default_value = "") const = 0;
    
    // 获取所有查询参数
    virtual std::unordered_map<std::string, std::string> get_query_params() const = 0;
    
    // 获取路径参数（例如 /clients/{client_id} 中的 client_id）
    virtual std::string get_path_param(const std::string& name, const std::string& default_value = "") const = 0;
    
    // 获取请求头
    virtual std::string get_header(const std::string& name, const std::string& default_value = "") const = 0;
    
    // 获取所有请求头
    virtual std::unordered_map<std::string, std::string> get_headers() const = 0;
    
    // 获取请求体
    virtual std::string get_body() const = 0;
    
    // 设置路径参数
    virtual void set_path_param(const std::string& name, const std::string& value) = 0;
    
    // 检查是否是JSON请求
    virtual bool is_json_content_type() const = 0;
};

// 简单的HTTP请求实现
class SimpleHttpRequest : public HttpRequest {
public:
    SimpleHttpRequest(const std::string& method,
                    const std::string& path,
                    const std::string& body = "");
    
    // 从HTTP请求字符串解析请求
    static std::unique_ptr<SimpleHttpRequest> parse(const std::string& request_str);
    
    // 实现HttpRequest接口
    std::string get_method() const override;
    std::string get_path() const override;
    std::string get_query_param(const std::string& name, const std::string& default_value = "") const override;
    std::unordered_map<std::string, std::string> get_query_params() const override;
    std::string get_path_param(const std::string& name, const std::string& default_value = "") const override;
    std::string get_header(const std::string& name, const std::string& default_value = "") const override;
    std::unordered_map<std::string, std::string> get_headers() const override;
    std::string get_body() const override;
    void set_path_param(const std::string& name, const std::string& value) override;
    bool is_json_content_type() const override;
    
    // 设置查询参数
    void set_query_param(const std::string& name, const std::string& value);
    
    // 设置请求头
    void set_header(const std::string& name, const std::string& value);
    
private:
    std::string method_;
    std::string path_;
    std::string body_;
    std::unordered_map<std::string, std::string> query_params_;
    std::unordered_map<std::string, std::string> path_params_;
    std::unordered_map<std::string, std::string> headers_;
    
    // 解析查询字符串
    void parse_query_string(const std::string& query_str);
    
    // 解析请求头
    void parse_headers(const std::vector<std::string>& header_lines);
};

} // namespace http
} // namespace api_quota

#endif // HTTP_REQUEST_HPP