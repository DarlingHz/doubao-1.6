#pragma once
#include "models/Transaction.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>
#include <chrono>

namespace accounting {

struct TransactionFilter {
    int accountId = 0;        // 0表示不过滤
    int categoryId = 0;       // 0表示不过滤
    std::string type;         // 空字符串表示不过滤
    double amountMin = 0.0;   // 0表示不限制最小值
    double amountMax = 0.0;   // 0表示不限制最大值
    std::chrono::system_clock::time_point fromTime;  // 默认时间点
    std::chrono::system_clock::time_point toTime;    // 默认时间点
    
    TransactionFilter() {
        // 初始化时间范围为极端值
        fromTime = std::chrono::system_clock::from_time_t(0);
        toTime = std::chrono::system_clock::now() + std::chrono::hours(24 * 365 * 100); // 100年后
    }
};

class TransactionDAO {
public:
    TransactionDAO(Database& db);
    
    // 创建交易记录
    bool create(const Transaction& transaction);
    
    // 根据ID获取交易记录
    std::optional<Transaction> getById(int id);
    
    // 查询交易记录，支持过滤、排序和分页
    std::vector<Transaction> query(const TransactionFilter& filter,
                                  const std::string& sortBy = "time", // time或amount
                                  bool sortDesc = true,
                                  int page = 1, int pageSize = 20);
    
    // 更新交易记录
    bool update(const Transaction& transaction);
    
    // 删除交易记录
    bool remove(int id);
    
    // 检查交易记录是否存在
    bool exists(int id);
    
    // 获取符合条件的交易记录总数
    int getCount(const TransactionFilter& filter);
    
    // 获取某月的收支统计
    std::pair<double, double> getMonthlySummary(const std::string& month);
    
    // 获取某月按分类的支出统计
    std::vector<std::pair<int, double>> getCategoryExpenses(const std::string& month);
    
private:
    Database& db_;
    
    // 从数据库行构造Transaction对象
    Transaction fromRow(sqlite3_stmt* stmt);
    
    // 构建查询SQL
    std::string buildQuerySQL(const TransactionFilter& filter,
                             const std::string& sortBy,
                             bool sortDesc,
                             bool forCount = false);
    
    // 绑定查询参数
    void bindQueryParams(sqlite3_stmt* stmt, const TransactionFilter& filter);
};

} // namespace accounting
