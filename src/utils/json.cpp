#include "utils/json.hpp"
#include <sstream>
#include <stdexcept>
#include <charconv>
#include <cctype>

namespace api_quota {
namespace utils {

// JsonValue类实现
bool JsonValue::as_boolean() const {
    if (is_boolean()) {
        return std::get<bool>(value_);
    }
    throw std::runtime_error("Not a boolean value");
}

int64_t JsonValue::as_integer() const {
    if (is_integer()) {
        return std::get<int64_t>(value_);
    }
    if (is_double()) {
        return static_cast<int64_t>(std::get<double>(value_));
    }
    throw std::runtime_error("Not a number value");
}

double JsonValue::as_double() const {
    if (is_double()) {
        return std::get<double>(value_);
    }
    if (is_integer()) {
        return static_cast<double>(std::get<int64_t>(value_));
    }
    throw std::runtime_error("Not a number value");
}

const std::string& JsonValue::as_string() const {
    if (is_string()) {
        return std::get<std::string>(value_);
    }
    throw std::runtime_error("Not a string value");
}

const std::vector<JsonValue>& JsonValue::as_array() const {
    if (is_array()) {
        return std::get<std::vector<JsonValue>>(value_);
    }
    throw std::runtime_error("Not an array value");
}

const std::unordered_map<std::string, JsonValue>& JsonValue::as_object() const {
    if (is_object()) {
        return std::get<std::unordered_map<std::string, JsonValue>>(value_);
    }
    throw std::runtime_error("Not an object value");
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (is_object()) {
        const auto& obj = as_object();
        auto it = obj.find(key);
        if (it != obj.end()) {
            return it->second;
        }
        throw std::runtime_error("Key not found: " + key);
    }
    throw std::runtime_error("Not an object");
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (!is_object()) {
        value_ = std::unordered_map<std::string, JsonValue>();
    }
    return std::get<std::unordered_map<std::string, JsonValue>>(value_)[key];
}

bool JsonValue::contains(const std::string& key) const {
    if (is_object()) {
        const auto& obj = as_object();
        return obj.find(key) != obj.end();
    }
    return false;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (is_array()) {
        const auto& arr = as_array();
        if (index < arr.size()) {
            return arr[index];
        }
        throw std::runtime_error("Index out of range");
    }
    throw std::runtime_error("Not an array");
}

JsonValue& JsonValue::operator[](size_t index) {
    if (!is_array()) {
        value_ = std::vector<JsonValue>();
    }
    auto& arr = std::get<std::vector<JsonValue>>(value_);
    if (index >= arr.size()) {
        arr.resize(index + 1);
    }
    return arr[index];
}

size_t JsonValue::size() const {
    if (is_array()) {
        return as_array().size();
    }
    if (is_object()) {
        return as_object().size();
    }
    throw std::runtime_error("Not an array or object");
}

void JsonValue::serialize_to_string(std::stringstream& ss) const {
    if (is_null()) {
        ss << "null";
    } else if (is_boolean()) {
        ss << (as_boolean() ? "true" : "false");
    } else if (is_integer()) {
        ss << as_integer();
    } else if (is_double()) {
        ss << as_double();
    } else if (is_string()) {
        const auto& str = as_string();
        ss << '"';
        for (char c : str) {
            switch (c) {
                case '"': ss << '\\' << '"'; break;
                case '\\': ss << '\\' << '\\'; break;
                case '\b': ss << '\\' << 'b'; break;
                case '\f': ss << '\\' << 'f'; break;
                case '\n': ss << '\\' << 'n'; break;
                case '\r': ss << '\\' << 'r'; break;
                case '\t': ss << '\\' << 't'; break;
                default: ss << c; break;
            }
        }
        ss << '"';
    } else if (is_array()) {
        const auto& arr = as_array();
        ss << '[';
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i].serialize_to_string(ss);
            if (i < arr.size() - 1) {
                ss << ", ";
            }
        }
        ss << ']';
    } else if (is_object()) {
        const auto& obj = as_object();
        ss << '{';
        bool first = true;
        for (const auto& [key, val] : obj) {
            if (!first) {
                ss << ", ";
            }
            first = false;
            JsonValue(key).serialize_to_string(ss);
            ss << ": ";
            val.serialize_to_string(ss);
        }
        ss << '}';
    }
}

std::string JsonValue::to_string() const {
    std::stringstream ss;
    serialize_to_string(ss);
    return ss.str();
}

// JSON解析器实现
class JsonParser {
public:
    explicit JsonParser(const std::string& str) : str_(str), pos_(0) {}
    
    std::optional<JsonValue> parse() {
        skip_whitespace();
        if (pos_ >= str_.size()) {
            return std::nullopt;
        }
        
        return parse_value();
    }
    
private:
    const std::string& str_;
    size_t pos_;
    
    void skip_whitespace() {
        while (pos_ < str_.size() && std::isspace(static_cast<unsigned char>(str_[pos_]))) {
            ++pos_;
        }
    }
    
    std::optional<JsonValue> parse_value() {
        skip_whitespace();
        if (pos_ >= str_.size()) {
            return std::nullopt;
        }
        
        char c = str_[pos_];
        if (c == '{') {
            return parse_object();
        } else if (c == '[') {
            return parse_array();
        } else if (c == '"') {
            return parse_string();
        } else if (c == 't') {
            return parse_literal("true", true);
        } else if (c == 'f') {
            return parse_literal("false", false);
        } else if (c == 'n') {
            return parse_literal("null", nullptr);
        } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            return parse_number();
        }
        
        return std::nullopt;
    }
    
    std::optional<JsonValue> parse_object() {
        ++pos_; // skip '{'
        std::unordered_map<std::string, JsonValue> obj;
        
        while (true) {
            skip_whitespace();
            if (pos_ >= str_.size()) {
                return std::nullopt;
            }
            
            if (str_[pos_] == '}') {
                ++pos_;
                return obj;
            }
            
            // 解析键
            auto key_opt = parse_string();
            if (!key_opt) {
                return std::nullopt;
            }
            std::string key = key_opt->as_string();
            
            skip_whitespace();
            if (pos_ >= str_.size() || str_[pos_] != ':') {
                return std::nullopt;
            }
            ++pos_;
            
            // 解析值
            auto value_opt = parse_value();
            if (!value_opt) {
                return std::nullopt;
            }
            
            obj[key] = *value_opt;
            
            skip_whitespace();
            if (pos_ >= str_.size()) {
                return std::nullopt;
            }
            
            if (str_[pos_] == '}') {
                ++pos_;
                return obj;
            } else if (str_[pos_] != ',') {
                return std::nullopt;
            }
            ++pos_;
        }
    }
    
    std::optional<JsonValue> parse_array() {
        ++pos_; // skip '['
        std::vector<JsonValue> arr;
        
        while (true) {
            skip_whitespace();
            if (pos_ >= str_.size()) {
                return std::nullopt;
            }
            
            if (str_[pos_] == ']') {
                ++pos_;
                return arr;
            }
            
            auto value_opt = parse_value();
            if (!value_opt) {
                return std::nullopt;
            }
            
            arr.push_back(*value_opt);
            
            skip_whitespace();
            if (pos_ >= str_.size()) {
                return std::nullopt;
            }
            
            if (str_[pos_] == ']') {
                ++pos_;
                return arr;
            } else if (str_[pos_] != ',') {
                return std::nullopt;
            }
            ++pos_;
        }
    }
    
    std::optional<JsonValue> parse_string() {
        ++pos_; // skip '"'
        std::string str;
        
        while (pos_ < str_.size()) {
            char c = str_[pos_];
            ++pos_;
            
            if (c == '"') {
                return str;
            } else if (c == '\\') {
                if (pos_ >= str_.size()) {
                    return std::nullopt;
                }
                c = str_[pos_];
                ++pos_;
                switch (c) {
                    case '"': str += '"'; break;
                    case '\\': str += '\\'; break;
                    case 'b': str += '\b'; break;
                    case 'f': str += '\f'; break;
                    case 'n': str += '\n'; break;
                    case 'r': str += '\r'; break;
                    case 't': str += '\t'; break;
                    default: str += c; break;
                }
            } else {
                str += c;
            }
        }
        
        return std::nullopt;
    }
    
    std::optional<JsonValue> parse_literal(const std::string& literal, const JsonValue& value) {
        if (str_.substr(pos_, literal.size()) == literal) {
            pos_ += literal.size();
            return value;
        }
        return std::nullopt;
    }
    
    std::optional<JsonValue> parse_number() {
        size_t start = pos_;
        if (str_[pos_] == '-') {
            ++pos_;
        }
        
        // 整数部分
        bool has_digit = false;
        while (pos_ < str_.size() && std::isdigit(static_cast<unsigned char>(str_[pos_]))) {
            ++pos_;
            has_digit = true;
        }
        
        if (!has_digit) {
            return std::nullopt;
        }
        
        // 小数部分
        bool is_double = false;
        if (pos_ < str_.size() && str_[pos_] == '.') {
            is_double = true;
            ++pos_;
            has_digit = false;
            while (pos_ < str_.size() && std::isdigit(static_cast<unsigned char>(str_[pos_]))) {
                ++pos_;
                has_digit = true;
            }
            if (!has_digit) {
                return std::nullopt;
            }
        }
        
        // 指数部分
        if (pos_ < str_.size() && (str_[pos_] == 'e' || str_[pos_] == 'E')) {
            is_double = true;
            ++pos_;
            if (pos_ < str_.size() && (str_[pos_] == '+' || str_[pos_] == '-')) {
                ++pos_;
            }
            has_digit = false;
            while (pos_ < str_.size() && std::isdigit(static_cast<unsigned char>(str_[pos_]))) {
                ++pos_;
                has_digit = true;
            }
            if (!has_digit) {
                return std::nullopt;
            }
        }
        
        std::string num_str = str_.substr(start, pos_ - start);
        if (is_double) {
            double d;
            auto result = std::from_chars(num_str.data(), num_str.data() + num_str.size(), d);
            if (result.ec == std::errc()) {
                return d;
            }
        } else {
            int64_t i;
            auto result = std::from_chars(num_str.data(), num_str.data() + num_str.size(), i);
            if (result.ec == std::errc()) {
                return i;
            }
        }
        
        return std::nullopt;
    }
};

std::optional<JsonValue> parse_json(const std::string& json_str) {
    JsonParser parser(json_str);
    return parser.parse();
}

std::string serialize_json(const JsonValue& value) {
    return value.to_string();
}

} // namespace utils
} // namespace api_quota