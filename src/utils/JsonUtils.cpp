#include "utils/JsonUtils.h"
#include <sstream>

namespace accounting {

// 简单的JSON字符串转义函数
std::string escapeJsonString(const std::string& input) {
    std::ostringstream output;
    for (char c : input) {
        switch (c) {
            case '\\': output << "\\\\";
            case '"': output << "\\\"";
            case '\n': output << "\\n";
            case '\r': output << "\\r";
            case '\t': output << "\\t";
            default:
                output << c;
        }
    }
    return output.str();
}

// JsonObject方法实现
std::string JsonObject::toString() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : values_) {
        if (!first) {
            oss << ", ";
        }
        oss << '"' << escapeJsonString(key) << '": "' << value->toString() << '"';
        first = false;
    }
    oss << "}";
    return oss.str();
}

void JsonObject::set(const std::string& key, const std::string& value) {
    class JsonString : public JsonValue {
    public:
        JsonString(const std::string& val) : value_(val) {}
        JsonType getType() const override { return JsonType::String; }
        std::string toString() const override {
            return '"' + escapeJsonString(value_) + '"';
        }
    private:
        std::string value_;
    };
    values_[key] = std::make_shared<JsonString>(value);
}

void JsonObject::set(const std::string& key, int value) {
    class JsonNumber : public JsonValue {
    public:
        JsonNumber(int val) : value_(val) {}
        JsonType getType() const override { return JsonType::Number; }
        std::string toString() const override {
            return std::to_string(value_);
        }
    private:
        int value_;
    };
    values_[key] = std::make_shared<JsonNumber>(value);
}

void JsonObject::set(const std::string& key, double value) {
    class JsonDouble : public JsonValue {
    public:
        JsonDouble(double val) : value_(val) {}
        JsonType getType() const override { return JsonType::Number; }
        std::string toString() const override {
            return std::to_string(value_);
        }
    private:
        double value_;
    };
    values_[key] = std::make_shared<JsonDouble>(value);
}

void JsonObject::set(const std::string& key, bool value) {
    class JsonBoolean : public JsonValue {
    public:
        JsonBoolean(bool val) : value_(val) {}
        JsonType getType() const override { return JsonType::Boolean; }
        std::string toString() const override {
            return value_ ? "true" : "false";
        }
    private:
        bool value_;
    };
    values_[key] = std::make_shared<JsonBoolean>(value);
}

void JsonObject::set(const std::string& key, std::shared_ptr<JsonValue> value) {
    values_[key] = value;
}

std::string JsonObject::getString(const std::string& /*key*/) const {
    // 简化实现，实际应该检查类型和存在性
    return "";
}

int JsonObject::getInt(const std::string& /*key*/) const {
    // 简化实现，实际应该检查类型和存在性
    return 0;
}

// JsonArray方法实现
std::string JsonArray::toString() const {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& value : values_) {
        if (!first) {
            oss << ", ";
        }
        oss << value->toString();
        first = false;
    }
    oss << "]";
    return oss.str();
}

void JsonArray::push_back(const std::string& value) {
    class JsonString : public JsonValue {
    public:
        JsonString(const std::string& val) : value_(val) {}
        JsonType getType() const override { return JsonType::String; }
        std::string toString() const override {
            return '"' + escapeJsonString(value_) + '"';
        }
    private:
        std::string value_;
    };
    values_.push_back(std::make_shared<JsonString>(value));
}

void JsonArray::push_back(int value) {
    class JsonNumber : public JsonValue {
    public:
        JsonNumber(int val) : value_(val) {}
        JsonType getType() const override { return JsonType::Number; }
        std::string toString() const override {
            return std::to_string(value_);
        }
    private:
        int value_;
    };
    values_.push_back(std::make_shared<JsonNumber>(value));
}

void JsonArray::push_back(double value) {
    class JsonDouble : public JsonValue {
    public:
        JsonDouble(double val) : value_(val) {}
        JsonType getType() const override { return JsonType::Number; }
        std::string toString() const override {
            return std::to_string(value_);
        }
    private:
        double value_;
    };
    values_.push_back(std::make_shared<JsonDouble>(value));
}

void JsonArray::push_back(bool value) {
    class JsonBoolean : public JsonValue {
    public:
        JsonBoolean(bool val) : value_(val) {}
        JsonType getType() const override { return JsonType::Boolean; }
        std::string toString() const override {
            return value_ ? "true" : "false";
        }
    private:
        bool value_;
    };
    values_.push_back(std::make_shared<JsonBoolean>(value));
}

void JsonArray::push_back(std::shared_ptr<JsonValue> value) {
    values_.push_back(value);
}

// JsonUtils方法实现
std::shared_ptr<JsonObject> JsonUtils::serializeAccount(const Account& account) {
    auto obj = std::make_shared<JsonObject>();
    obj->set("id", account.getId());
    obj->set("name", account.getName());
    // 简化实现，只设置必要字段
    obj->set("type", "normal"); // 假设默认类型为normal
    obj->set("balance", 0.0);
    obj->set("description", "");
    return obj;
}

std::shared_ptr<JsonArray> JsonUtils::serializeAccounts(const std::vector<Account>& accounts) {
    auto array = std::make_shared<JsonArray>();
    for (const auto& account : accounts) {
        array->push_back(serializeAccount(account));
    }
    return array;
}

std::shared_ptr<JsonObject> JsonUtils::createErrorResponse(const std::string& code, const std::string& message) {
    auto obj = std::make_shared<JsonObject>();
    obj->set("code", code);
    obj->set("message", message);
    return obj;
}

std::string JsonUtils::jsonToString(std::shared_ptr<JsonValue> val) {
    return val->toString();
}

} // namespace accounting