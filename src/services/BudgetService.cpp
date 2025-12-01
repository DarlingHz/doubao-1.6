#include "services/BudgetService.h"
#include <stdexcept>
#include <iostream>
#include <regex>

namespace accounting {

BudgetService::BudgetService(Database& db) : budgetDAO_(db), categoryDAO_(db) {
}

bool BudgetService::setBudget(const std::string& month, const std::vector<BudgetItem>& items) {
    if (!validateMonthFormat(month)) {
        std::cerr << "Invalid month format" << std::endl;
        return false;
    }
    
    if (items.empty()) {
        std::cerr << "No budget items provided" << std::endl;
        return false;
    }
    
    // 开启事务处理
    // 注意：这里简化处理，实际应该使用数据库事务
    bool allSuccess = true;
    
    for (const auto& item : items) {
        // 检查分类是否存在且为支出类型
        auto category = categoryDAO_.getById(item.categoryId);
        if (!category || category->getType() != "expense") {
            std::cerr << "Invalid category for budget: " << item.categoryId << std::endl;
            allSuccess = false;
            continue;
        }
        
        // 创建预算对象
        Budget budget(0, item.categoryId, month, item.limit);
        
        if (!validateBudget(budget)) {
            std::cerr << "Invalid budget data" << std::endl;
            allSuccess = false;
            continue;
        }
        
        if (!budgetDAO_.upsert(budget)) {
            std::cerr << "Failed to upsert budget for category: " << item.categoryId << std::endl;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::optional<Budget> BudgetService::getBudgetById(int id) {
    if (id <= 0) {
        return std::nullopt;
    }
    
    return budgetDAO_.getById(id);
}

std::vector<Budget> BudgetService::getBudgetsByMonth(const std::string& month) {
    if (!validateMonthFormat(month)) {
        return {};
    }
    
    return budgetDAO_.getByMonth(month);
}

bool BudgetService::deleteBudget(int id) {
    if (id <= 0) {
        return false;
    }
    
    if (!budgetDAO_.exists(id)) {
        return false;
    }
    
    return budgetDAO_.remove(id);
}

bool BudgetService::validateBudget(const Budget& budget) {
    // 验证分类ID
    if (budget.getCategoryId() <= 0) {
        return false;
    }
    
    // 验证月份格式
    if (!validateMonthFormat(budget.getMonth())) {
        return false;
    }
    
    // 验证预算限额
    if (budget.getLimit() < 0) {
        return false;
    }
    
    return true;
}

bool BudgetService::validateMonthFormat(const std::string& month) {
    // 验证月份格式: YYYY-MM
    std::regex monthRegex(R"(\d{4}-(0[1-9]|1[0-2]))");
    return std::regex_match(month, monthRegex);
}

} // namespace accounting
