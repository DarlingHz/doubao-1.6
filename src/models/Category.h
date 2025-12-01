#pragma once
#include <string>

namespace accounting {

class Category {
public:
    Category(int id = 0, const std::string& name = "", const std::string& type = "");
    
    int getId() const;
    void setId(int id);
    
    const std::string& getName() const;
    void setName(const std::string& name);
    
    const std::string& getType() const;
    void setType(const std::string& type);
    
    bool isValid() const;
    
private:
    int id_;              // 分类ID
    std::string name_;    // 分类名称
    std::string type_;    // 分类类型 (income, expense)
};

} // namespace accounting
