#pragma once
#include "models/Summary.h"
#include "dao/TransactionDAO.h"
#include "dao/CategoryDAO.h"
#include "dao/BudgetDAO.h"
#include "utils/Cache.h"
#include "dao/Database.h"
#include <vector>
#include <string>

namespace accounting {

class SummaryService {
public:
    SummaryService(Database& db);
    
    // 获取月度汇总
    MonthlySummary getMonthlySummary(const std::string& month);
    
    // 获取趋势统计
    std::vector<TrendData> getTrendSummary(const std::string& fromMonth, const std::string& toMonth);
    
    // 清除缓存（当相关数据更新时调用）
    void clearCache(const std::string& month = "");
    
private:
    TransactionDAO transactionDAO_;
    CategoryDAO categoryDAO_;
    BudgetDAO budgetDAO_;
    Cache<std::string, MonthlySummary> monthlySummaryCache_;
    
    // 验证月份格式
    bool validateMonthFormat(const std::string& month);
    
    // 生成月份列表
    std::vector<std::string> generateMonthList(const std::string& fromMonth, const std::string& toMonth);
    
    // 构建月度汇总数据
    MonthlySummary buildMonthlySummary(const std::string& month);
};

} // namespace accounting
