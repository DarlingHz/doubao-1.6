#pragma once
#include "models/Budget.h"
#include "dao/BudgetDAO.h"
#include "dao/CategoryDAO.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>

namespace accounting {

struct BudgetItem {
    int categoryId;
    double limit;
    
    BudgetItem(int catId = 0, double lim = 0.0) : categoryId(catId), limit(lim) {}
};

class BudgetService {
public:
    BudgetService(Database& db);
    
    // 设置或更新预算
    bool setBudget(const std::string& month, const std::vector<BudgetItem>& items);
    
    // 获取预算详情
    std::optional<Budget> getBudgetById(int id);
    
    // 获取某月的预算设置
    std::vector<Budget> getBudgetsByMonth(const std::string& month);
    
    // 删除预算
    bool deleteBudget(int id);
    
private:
    BudgetDAO budgetDAO_;
    CategoryDAO categoryDAO_;
    
    // 验证预算数据
    bool validateBudget(const Budget& budget);
    
    // 验证月份格式
    bool validateMonthFormat(const std::string& month);
};

} // namespace accounting
