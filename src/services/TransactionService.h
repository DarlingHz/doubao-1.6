#pragma once
#include "models/Transaction.h"
#include "dao/TransactionDAO.h"
#include "dao/AccountDAO.h"
#include "dao/CategoryDAO.h"
#include "dao/Database.h"
#include <vector>
#include <optional>
#include <string>

namespace accounting {

struct TransactionQueryResult {
    std::vector<Transaction> transactions;
    int totalCount;
    int page;
    int pageSize;
    
    TransactionQueryResult(const std::vector<Transaction>& trans = {},
                          int count = 0, int pg = 1, int pgSize = 20)
        : transactions(trans), totalCount(count), page(pg), pageSize(pgSize) {}
};

class TransactionService {
public:
    TransactionService(Database& db);
    
    // 创建交易记录
    std::optional<Transaction> createTransaction(const Transaction& transaction);
    
    // 获取交易记录详情
    std::optional<Transaction> getTransactionById(int id);
    
    // 查询交易记录
    TransactionQueryResult queryTransactions(const TransactionFilter& filter,
                                          const std::string& sortBy = "time",
                                          bool sortDesc = true,
                                          int page = 1, int pageSize = 20);
    
    // 更新交易记录
    bool updateTransaction(const Transaction& transaction);
    
    // 删除交易记录
    bool deleteTransaction(int id);
    
private:
    TransactionDAO transactionDAO_;
    AccountDAO accountDAO_;
    CategoryDAO categoryDAO_;
    
    // 验证交易数据
    bool validateTransaction(const Transaction& transaction);
    
    // 检查关联资源是否存在
    bool checkRelatedResources(const Transaction& transaction);
};

} // namespace accounting
