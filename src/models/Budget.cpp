#include "models/Budget.h"
#include <regex>

namespace accounting {

Budget::Budget(int id, int categoryId, const std::string& month, double limit)
    : id_(id), categoryId_(categoryId), month_(month), limit_(limit) {
}

int Budget::getId() const {
    return id_;
}

void Budget::setId(int id) {
    id_ = id;
}

int Budget::getCategoryId() const {
    return categoryId_;
}

void Budget::setCategoryId(int categoryId) {
    categoryId_ = categoryId;
}

const std::string& Budget::getMonth() const {
    return month_;
}

void Budget::setMonth(const std::string& month) {
    month_ = month;
}

double Budget::getLimit() const {
    return limit_;
}

void Budget::setLimit(double limit) {
    limit_ = limit;
}

bool Budget::isValid() const {
    // 验证月份格式: YYYY-MM
    std::regex monthRegex(R"(\d{4}-(0[1-9]|1[0-2]))");
    return categoryId_ > 0 && 
           std::regex_match(month_, monthRegex) && 
           limit_ >= 0.0;
}

} // namespace accounting
