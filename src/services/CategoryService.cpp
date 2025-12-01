#include "services/CategoryService.h"
#include <stdexcept>
#include <iostream>

namespace accounting {

CategoryService::CategoryService(Database& db) : dao_(db) {
}

std::optional<Category> CategoryService::createCategory(const Category& category) {
    if (!validateCategory(category)) {
        std::cerr << "Invalid category data" << std::endl;
        return std::nullopt;
    }
    
    if (!dao_.create(category)) {
        std::cerr << "Failed to create category" << std::endl;
        return std::nullopt;
    }
    
    // 这里简化处理，实际应该通过其他方式获取刚插入的记录
    std::vector<Category> categories = dao_.getAll(category.getType());
    if (!categories.empty()) {
        return categories.back(); // 返回最后一个，假设是刚插入的
    }
    
    return std::nullopt;
}

std::optional<Category> CategoryService::getCategoryById(int id) {
    if (id <= 0) {
        return std::nullopt;
    }
    
    return dao_.getById(id);
}

std::vector<Category> CategoryService::getCategories(const std::string& type) {
    return dao_.getAll(type);
}

bool CategoryService::updateCategory(const Category& category) {
    if (category.getId() <= 0) {
        return false;
    }
    
    if (!validateCategory(category)) {
        return false;
    }
    
    if (!dao_.exists(category.getId())) {
        return false;
    }
    
    return dao_.update(category);
}

bool CategoryService::deleteCategory(int id) {
    if (id <= 0) {
        return false;
    }
    
    if (!dao_.exists(id)) {
        return false;
    }
    
    return dao_.remove(id);
}

bool CategoryService::validateCategory(const Category& category) {
    // 验证分类名称
    if (category.getName().empty()) {
        return false;
    }
    
    // 验证分类类型
    const std::string& type = category.getType();
    if (type != "income" && type != "expense") {
        return false;
    }
    
    return true;
}

} // namespace accounting
