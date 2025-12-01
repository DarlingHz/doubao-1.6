#include "services/SummaryService.h"
#include <stdexcept>
#include <iostream>
#include <regex>
#include <map>
#include <sstream>
#include <iomanip>

namespace accounting {

SummaryService::SummaryService(Database& db)
    : transactionDAO_(db), categoryDAO_(db), budgetDAO_(db) {
}

MonthlySummary SummaryService::getMonthlySummary(const std::string& month) {
    if (!validateMonthFormat(month)) {
        throw std::invalid_argument("Invalid month format. Expected YYYY-MM");
    }
    
    // 尝试从缓存获取
    auto cachedSummary = monthlySummaryCache_.get(month);
    if (cachedSummary) {
        return *cachedSummary;
    }
    
    // 构建汇总数据
    MonthlySummary summary = buildMonthlySummary(month);
    
    // 缓存结果
    monthlySummaryCache_.put(month, summary);
    
    return summary;
}

std::vector<TrendData> SummaryService::getTrendSummary(const std::string& fromMonth, const std::string& toMonth) {
    if (!validateMonthFormat(fromMonth) || !validateMonthFormat(toMonth)) {
        throw std::invalid_argument("Invalid month format. Expected YYYY-MM");
    }
    
    // 生成月份列表
    std::vector<std::string> months = generateMonthList(fromMonth, toMonth);
    std::vector<TrendData> trendData;
    
    for (const auto& month : months) {
        auto [income, expense] = transactionDAO_.getMonthlySummary(month);
        trendData.emplace_back(month, income, expense);
    }
    
    return trendData;
}

void SummaryService::clearCache(const std::string& month) {
    if (month.empty()) {
        monthlySummaryCache_.clear();
    } else {
        monthlySummaryCache_.remove(month);
    }
}

bool SummaryService::validateMonthFormat(const std::string& month) {
    std::regex monthRegex(R"(\d{4}-(0[1-9]|1[0-2]))");
    return std::regex_match(month, monthRegex);
}

std::vector<std::string> SummaryService::generateMonthList(const std::string& fromMonth, const std::string& toMonth) {
    std::vector<std::string> months;
    
    // 解析起始月份
    int fromYear = std::stoi(fromMonth.substr(0, 4));
    int fromMonthNum = std::stoi(fromMonth.substr(5, 2));
    
    // 解析结束月份
    int toYear = std::stoi(toMonth.substr(0, 4));
    int toMonthNum = std::stoi(toMonth.substr(5, 2));
    
    // 生成月份列表
    int currentYear = fromYear;
    int currentMonth = fromMonthNum;
    
    while (true) {
        std::stringstream ss;
        ss << currentYear << "-" << std::setw(2) << std::setfill('0') << currentMonth;
        months.push_back(ss.str());
        
        // 检查是否达到结束月份
        if (currentYear == toYear && currentMonth == toMonthNum) {
            break;
        }
        
        // 增加一个月
        currentMonth++;
        if (currentMonth > 12) {
            currentMonth = 1;
            currentYear++;
        }
        
        // 防止无限循环
        if (currentYear > toYear + 1) {
            break;
        }
    }
    
    return months;
}

MonthlySummary SummaryService::buildMonthlySummary(const std::string& month) {
    MonthlySummary summary;
    summary.month = month;
    
    // 获取月度收支总额
    auto [income, expense] = transactionDAO_.getMonthlySummary(month);
    summary.totalIncome = income;
    summary.totalExpense = expense;
    summary.balance = income - expense;
    
    // 获取按分类的支出统计
    auto categoryExpenses = transactionDAO_.getCategoryExpenses(month);
    
    // 获取所有支出分类
    auto expenseCategories = categoryDAO_.getAll("expense");
    
    // 获取该月的预算设置
    auto budgets = budgetDAO_.getByMonth(month);
    
    // 创建预算映射，方便查找
    std::map<int, double> budgetMap;
    for (const auto& budget : budgets) {
        budgetMap[budget.getCategoryId()] = budget.getLimit();
    }
    
    // 创建分类映射，方便查找分类名称
    std::map<int, std::string> categoryNameMap;
    for (const auto& category : expenseCategories) {
        categoryNameMap[category.getId()] = category.getName();
    }
    
    // 构建分类汇总数据
    for (const auto& [categoryId, expAmount] : categoryExpenses) {
        CategorySummary catSummary;
        catSummary.categoryId = categoryId;
        catSummary.categoryName = categoryNameMap[categoryId];
        catSummary.expense = expAmount;
        
        // 检查是否有预算
        if (budgetMap.count(categoryId)) {
            catSummary.budgetLimit = budgetMap[categoryId];
            catSummary.exceed = expAmount > budgetMap[categoryId];
        } else {
            catSummary.budgetLimit = 0.0;
            catSummary.exceed = false;
        }
        
        summary.perCategory.push_back(catSummary);
    }
    
    // 添加有预算但没有支出的分类
    for (const auto& budget : budgets) {
        bool hasExpense = false;
        for (const auto& catExp : categoryExpenses) {
            if (catExp.first == budget.getCategoryId()) {
                hasExpense = true;
                break;
            }
        }
        
        if (!hasExpense) {
            CategorySummary catSummary;
            catSummary.categoryId = budget.getCategoryId();
            catSummary.categoryName = categoryNameMap[budget.getCategoryId()];
            catSummary.expense = 0.0;
            catSummary.budgetLimit = budget.getLimit();
            catSummary.exceed = false;
            
            summary.perCategory.push_back(catSummary);
        }
    }
    
    return summary;
}

} // namespace accounting
