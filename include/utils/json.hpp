#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <optional>

namespace api_quota {
namespace utils {

class JsonValue {
public:
    using ValueType = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        std::vector<JsonValue>,
        std::unordered_map<std::string, JsonValue>
    >;

    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int64_t i) : value_(i) {}
    JsonValue(double d) : value_(d) {}
    JsonValue(const std::string& s) : value_(s) {}
    JsonValue(std::string&& s) : value_(std::move(s)) {}
    JsonValue(const std::vector<JsonValue>& arr) : value_(arr) {}
    JsonValue(std::vector<JsonValue>&& arr) : value_(std::move(arr)) {}
    JsonValue(const std::unordered_map<std::string, JsonValue>& obj) : value_(obj) {}
    JsonValue(std::unordered_map<std::string, JsonValue>&& obj) : value_(std::move(obj)) {}

    // 类型检查方法
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool is_boolean() const { return std::holds_alternative<bool>(value_); }
    bool is_number() const { return std::holds_alternative<int64_t>(value_) || std::holds_alternative<double>(value_); }
    bool is_integer() const { return std::holds_alternative<int64_t>(value_); }
    bool is_double() const { return std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_array() const { return std::holds_alternative<std::vector<JsonValue>>(value_); }
    bool is_object() const { return std::holds_alternative<std::unordered_map<std::string, JsonValue>>(value_); }

    // 类型转换方法
    bool as_boolean() const;  
    int64_t as_integer() const;  
    double as_double() const;  
    const std::string& as_string() const;  
    const std::vector<JsonValue>& as_array() const;  
    const std::unordered_map<std::string, JsonValue>& as_object() const;  

    // 访问对象成员
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](const std::string& key);
    bool contains(const std::string& key) const;

    // 访问数组元素
    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);
    size_t size() const;

    // 转换为JSON字符串
    std::string to_string() const;

private:
    ValueType value_;
    void serialize_to_string(std::stringstream& ss) const;
};

// JSON解析函数
extern std::optional<JsonValue> parse_json(const std::string& json_str);

// JSON序列化函数
extern std::string serialize_json(const JsonValue& value);

} // namespace utils
} // namespace api_quota

#endif // JSON_HPP