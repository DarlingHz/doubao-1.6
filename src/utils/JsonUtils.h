#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include "models/Account.h"

namespace accounting {

// 简化的JSON值类型枚举
enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Object,
    Array
};

// 简化的JSON值类
class JsonValue {
public:
    virtual ~JsonValue() = default;
    virtual JsonType getType() const = 0;
    virtual std::string toString() const = 0;
};

// 简化的JSON对象类
class JsonObject : public JsonValue {
public:
    JsonType getType() const override { return JsonType::Object; }
    std::string toString() const override;
    
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, int value);
    void set(const std::string& key, double value);
    void set(const std::string& key, bool value);
    void set(const std::string& key, std::shared_ptr<JsonValue> value);
    
    std::string getString(const std::string& key) const;
    int getInt(const std::string& key) const;
    double getDouble(const std::string& key) const;
    bool getBool(const std::string& key) const;
    std::shared_ptr<JsonObject> getObject(const std::string& key) const;
    
private:
    std::map<std::string, std::shared_ptr<JsonValue>> values_;
};

// 简化的JSON数组类
class JsonArray : public JsonValue {
public:
    JsonType getType() const override { return JsonType::Array; }
    std::string toString() const override;
    
    void push_back(const std::string& value);
    void push_back(int value);
    void push_back(double value);
    void push_back(bool value);
    void push_back(std::shared_ptr<JsonValue> value);
    
    size_t size() const { return values_.size(); }
    
private:
    std::vector<std::shared_ptr<JsonValue>> values_;
};

class JsonUtils {
public:
    // 简化的JSON序列化方法
    static std::shared_ptr<JsonObject> serializeAccount(const Account& account);
    static std::shared_ptr<JsonArray> serializeAccounts(const std::vector<Account>& accounts);
    
    // 错误响应生成
    static std::shared_ptr<JsonObject> createErrorResponse(const std::string& code, const std::string& message);
    
    // 通用工具方法
    static std::string jsonToString(std::shared_ptr<JsonValue> val);
};

} // namespace accounting
