#pragma once
#include <string>
#include <vector>

namespace accounting {

struct CategorySummary {
    int categoryId;
    std::string categoryName;
    double expense;
    double budgetLimit;
    bool exceed;
    
    CategorySummary(int catId = 0, const std::string& catName = "",
                   double exp = 0.0, double limit = 0.0, bool ex = false)
        : categoryId(catId), categoryName(catName), expense(exp),
          budgetLimit(limit), exceed(ex) {}
};

struct MonthlySummary {
    std::string month;
    double totalIncome;
    double totalExpense;
    double balance;
    std::vector<CategorySummary> perCategory;
    
    MonthlySummary(const std::string& m = "", double income = 0.0, 
                  double expense = 0.0, double bal = 0.0)
        : month(m), totalIncome(income), totalExpense(expense), balance(bal) {}
};

struct TrendData {
    std::string month;
    double totalIncome;
    double totalExpense;
    
    TrendData(const std::string& m = "", double income = 0.0, double expense = 0.0)
        : month(m), totalIncome(income), totalExpense(expense) {}
};

} // namespace accounting
