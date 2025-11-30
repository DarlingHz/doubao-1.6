#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <map>
#include "database.h"
#include "auth.h"

// 简单的JSON工具类
enum class JsonType {
    Object,
    Array,
    String,
    Number,
    Boolean,
    Null
};

class JsonValue {
public:
    JsonValue();
    JsonValue(const std::string& str);
    JsonValue(int num);
    JsonValue(bool b);
    JsonValue(JsonType type);
    
    // 获取值的方法
    bool isObject() const { return type_ == JsonType::Object; }
    bool isArray() const { return type_ == JsonType::Array; }
    bool isString() const { return type_ == JsonType::String; }
    bool isNumber() const { return type_ == JsonType::Number; }
    bool isBoolean() const { return type_ == JsonType::Boolean; }
    bool isNull() const { return type_ == JsonType::Null; }
    
    // 对象操作
    void add(const std::string& key, const JsonValue& value);
    JsonValue get(const std::string& key) const;
    
    // 数组操作
    void push(const JsonValue& value);
    JsonValue at(size_t index) const;
    size_t size() const;
    
    // 转换方法
    std::string toString() const;
    int toInt() const;
    bool toBool() const;
    
    // 解析方法
    static JsonValue parse(const std::string& json_str);
    
private:
    JsonType type_;
    std::string string_value_;
    int number_value_;
    bool bool_value_;
    std::map<std::string, JsonValue> object_value_;
    std::vector<JsonValue> array_value_;
};

using json = JsonValue;

// HTTP请求结构体
struct HttpRequest {
    std::string method;
    std::string path;
    std::string http_version; // 新增HTTP版本字段
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
    json body;
};

// HTTP响应结构体
struct HttpResponse {
    int status_code;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string status_message; // 新增状态消息字段
    
    HttpResponse() : status_code(200) {
        headers["Content-Type"] = "application/json";
    }
};

// 路由处理器类型
typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

// HTTP服务器类
class HttpServer {
public:
    HttpServer(int port, std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service);
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 注册路由
    void registerRoute(const std::string& method, const std::string& path, const RouteHandler& handler);
    
private:
    int port_;
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthService> auth_service_;
    std::unordered_map<std::string, RouteHandler> routes_;
    bool running_;
    
    // 处理请求
    HttpResponse handleRequest(const HttpRequest& request);
    
    // 解析请求路径中的参数
    std::string extractPathParam(const std::string& path_pattern, const std::string& actual_path, const std::string& param_name);
    
    // 检查路径匹配
    bool matchPath(const std::string& pattern, const std::string& path);
};

// 路由控制器基类
class Controller {
public:
    Controller(std::shared_ptr<Database> db, std::shared_ptr<AuthService> auth_service)
        : db_(db), auth_service_(auth_service) {}
    
    virtual ~Controller() = default;
    
protected:
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthService> auth_service_;
    
    // 创建成功响应
    HttpResponse successResponse(const json& data);
    
    // 创建错误响应
    HttpResponse errorResponse(int code, const std::string& message);
    
    // 验证认证Token
    std::optional<User> authenticateRequest(const HttpRequest& request);
    
    // 解析JSON请求体
    bool parseRequestBody(const HttpRequest& request, JsonValue& body);
};

#endif // HTTP_SERVER_H