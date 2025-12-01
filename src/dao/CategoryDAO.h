#pragma once
#include "models/Category.h"
#include "dao/Database.h"
#include <vector>
#include <optional>

namespace accounting {

class CategoryDAO {
public:
    CategoryDAO(Database& db);
    
    // 创建分类
    bool create(const Category& category);
    
    // 根据ID获取分类
    std::optional<Category> getById(int id);
    
    // 获取所有分类，支持类型过滤
    std::vector<Category> getAll(const std::string& type = "");
    
    // 更新分类
    bool update(const Category& category);
    
    // 删除分类
    bool remove(int id);
    
    // 检查分类是否存在
    bool exists(int id);
    
private:
    Database& db_;
    
    // 从数据库行构造Category对象
    Category fromRow(sqlite3_stmt* stmt);
};

} // namespace accounting
