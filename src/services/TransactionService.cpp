#include "services/TransactionService.h"
#include <stdexcept>
#include <iostream>

namespace accounting {

TransactionService::TransactionService(Database& db)
    : transactionDAO_(db), accountDAO_(db), categoryDAO_(db) {
}

std::optional<Transaction> TransactionService::createTransaction(const Transaction& transaction) {
    if (!validateTransaction(transaction)) {
        std::cerr << "Invalid transaction data" << std::endl;
        return std::nullopt;
    }
    
    if (!checkRelatedResources(transaction)) {
        std::cerr << "Related resources not found" << std::endl;
        return std::nullopt;
    }
    
    if (!transactionDAO_.create(transaction)) {
        std::cerr << "Failed to create transaction" << std::endl;
        return std::nullopt;
    }
    
    // 这里简化处理，实际应该通过其他方式获取刚插入的记录
    TransactionFilter filter;
    filter.fromTime = transaction.getTime() - std::chrono::minutes(1);
    filter.toTime = transaction.getTime() + std::chrono::minutes(1);
    
    auto results = queryTransactions(filter, "time", true, 1, 1);
    if (!results.transactions.empty()) {
        return results.transactions[0];
    }
    
    return std::nullopt;
}

std::optional<Transaction> TransactionService::getTransactionById(int id) {
    if (id <= 0) {
        return std::nullopt;
    }
    
    return transactionDAO_.getById(id);
}

TransactionQueryResult TransactionService::queryTransactions(const TransactionFilter& filter,
                                                          const std::string& sortBy,
                                                          bool sortDesc,
                                                          int page, int pageSize) {
    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 100) pageSize = 20;
    
    auto transactions = transactionDAO_.query(filter, sortBy, sortDesc, page, pageSize);
    int totalCount = transactionDAO_.getCount(filter);
    
    return TransactionQueryResult(transactions, totalCount, page, pageSize);
}

bool TransactionService::updateTransaction(const Transaction& transaction) {
    if (transaction.getId() <= 0) {
        return false;
    }
    
    if (!validateTransaction(transaction)) {
        return false;
    }
    
    if (!checkRelatedResources(transaction)) {
        return false;
    }
    
    if (!transactionDAO_.exists(transaction.getId())) {
        return false;
    }
    
    return transactionDAO_.update(transaction);
}

bool TransactionService::deleteTransaction(int id) {
    if (id <= 0) {
        return false;
    }
    
    if (!transactionDAO_.exists(id)) {
        return false;
    }
    
    return transactionDAO_.remove(id);
}

bool TransactionService::validateTransaction(const Transaction& transaction) {
    // 验证交易类型
    const std::string& type = transaction.getType();
    if (type != "income" && type != "expense") {
        return false;
    }
    
    // 验证金额
    if (transaction.getAmount() <= 0) {
        return false;
    }
    
    // 验证时间
    // 这里可以添加更多时间验证逻辑
    
    return true;
}

bool TransactionService::checkRelatedResources(const Transaction& transaction) {
    // 检查账户是否存在
    if (!accountDAO_.exists(transaction.getAccountId())) {
        return false;
    }
    
    // 检查分类是否存在
    auto category = categoryDAO_.getById(transaction.getCategoryId());
    if (!category) {
        return false;
    }
    
    // 检查分类类型是否匹配交易类型
    if (category->getType() != transaction.getType()) {
        return false;
    }
    
    return true;
}

} // namespace accounting
