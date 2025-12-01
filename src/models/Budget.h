#pragma once
#include <string>

namespace accounting {

class Budget {
public:
    Budget(int id = 0, int categoryId = 0, const std::string& month = "", double limit = 0.0);
    
    int getId() const;
    void setId(int id);
    
    int getCategoryId() const;
    void setCategoryId(int categoryId);
    
    const std::string& getMonth() const;
    void setMonth(const std::string& month);
    
    double getLimit() const;
    void setLimit(double limit);
    
    bool isValid() const;
    
private:
    int id_;              // 预算ID
    int categoryId_;      // 分类ID
    std::string month_;   // 月份 (格式: YYYY-MM)
    double limit_;        // 预算限额
};

} // namespace accounting
