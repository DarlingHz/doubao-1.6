#pragma once
#include "models/Category.h"
#include "dao/CategoryDAO.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>

namespace accounting {

class CategoryService {
public:
    CategoryService(Database& db);
    
    // 创建分类
    std::optional<Category> createCategory(const Category& category);
    
    // 获取分类详情
    std::optional<Category> getCategoryById(int id);
    
    // 获取分类列表
    std::vector<Category> getCategories(const std::string& type = "");
    
    // 更新分类
    bool updateCategory(const Category& category);
    
    // 删除分类
    bool deleteCategory(int id);
    
private:
    CategoryDAO dao_;
    
    // 验证分类数据
    bool validateCategory(const Category& category);
};

} // namespace accounting
