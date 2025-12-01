#pragma once
#include <string>

namespace accounting {

class Account {
public:
    Account(int id = 0, const std::string& name = "", 
            const std::string& type = "", double initialBalance = 0.0);
    
    int getId() const;    
    void setId(int id);
    
    const std::string& getName() const;
    void setName(const std::string& name);
    
    const std::string& getType() const;
    void setType(const std::string& type);
    
    double getInitialBalance() const;
    void setInitialBalance(double balance);
    
    double getCurrentBalance() const;
    void setCurrentBalance(double balance);
    
    bool isValid() const;
    
private:
    int id_;              // 账户ID
    std::string name_;    // 账户名称
    std::string type_;    // 账户类型 (cash, bank, wallet)
    double initialBalance_; // 初始余额
    double currentBalance_; // 当前余额 (运行时计算)
};

} // namespace accounting
