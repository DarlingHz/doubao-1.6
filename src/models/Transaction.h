#pragma once
#include <string>
#include <chrono>

namespace accounting {

class Transaction {
public:
    Transaction(int id = 0, int accountId = 0, int categoryId = 0,
               const std::string& type = "", double amount = 0.0,
               const std::chrono::system_clock::time_point& time = std::chrono::system_clock::now(),
               const std::string& note = "");
    
    int getId() const;
    void setId(int id);
    
    int getAccountId() const;
    void setAccountId(int accountId);
    
    int getCategoryId() const;
    void setCategoryId(int categoryId);
    
    const std::string& getType() const;
    void setType(const std::string& type);
    
    double getAmount() const;
    void setAmount(double amount);
    
    const std::chrono::system_clock::time_point& getTime() const;
    void setTime(const std::chrono::system_clock::time_point& time);
    
    const std::string& getNote() const;
    void setNote(const std::string& note);
    
    // 时间转换方法
    std::string timeToString() const;
    static std::chrono::system_clock::time_point stringToTime(const std::string& timeStr);
    
    bool isValid() const;
    
private:
    int id_;                      // 交易ID
    int accountId_;               // 账户ID
    int categoryId_;              // 分类ID
    std::string type_;            // 交易类型 (income, expense)
    double amount_;               // 金额
    std::chrono::system_clock::time_point time_; // 交易时间
    std::string note_;            // 备注
};

} // namespace accounting
