#pragma once
#include "models/Budget.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>

namespace accounting {

class BudgetDAO {
public:
    BudgetDAO(Database& db);
    
    // 创建或更新预算
    bool upsert(const Budget& budget);
    
    // 根据ID获取预算
    std::optional<Budget> getById(int id);
    
    // 根据月份和分类ID获取预算
    std::optional<Budget> getByMonthAndCategory(const std::string& month, int categoryId);
    
    // 获取某月的所有预算
    std::vector<Budget> getByMonth(const std::string& month);
    
    // 删除预算
    bool remove(int id);
    
    // 检查预算是否存在
    bool exists(int id);
    
private:
    Database& db_;
    
    // 从数据库行构造Budget对象
    Budget fromRow(sqlite3_stmt* stmt);
};

} // namespace accounting
