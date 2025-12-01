#include "models/Transaction.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace accounting {

Transaction::Transaction(int id, int accountId, int categoryId,
                       const std::string& type, double amount,
                       const std::chrono::system_clock::time_point& time,
                       const std::string& note)
    : id_(id), accountId_(accountId), categoryId_(categoryId),
      type_(type), amount_(amount), time_(time), note_(note) {
}

int Transaction::getId() const {
    return id_;
}

void Transaction::setId(int id) {
    id_ = id;
}

int Transaction::getAccountId() const {
    return accountId_;
}

void Transaction::setAccountId(int accountId) {
    accountId_ = accountId;
}

int Transaction::getCategoryId() const {
    return categoryId_;
}

void Transaction::setCategoryId(int categoryId) {
    categoryId_ = categoryId;
}

const std::string& Transaction::getType() const {
    return type_;
}

void Transaction::setType(const std::string& type) {
    type_ = type;
}

double Transaction::getAmount() const {
    return amount_;
}

void Transaction::setAmount(double amount) {
    amount_ = amount;
}

const std::chrono::system_clock::time_point& Transaction::getTime() const {
    return time_;
}

void Transaction::setTime(const std::chrono::system_clock::time_point& time) {
    time_ = time;
}

const std::string& Transaction::getNote() const {
    return note_;
}

void Transaction::setNote(const std::string& note) {
    note_ = note;
}

std::string Transaction::timeToString() const {
    auto tp = std::chrono::system_clock::to_time_t(time_);
    std::tm tm = *std::localtime(&tp);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point Transaction::stringToTime(const std::string& timeStr) {
    std::tm tm = {};
    std::stringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        throw std::invalid_argument("Invalid time format. Expected ISO8601: YYYY-MM-DDTHH:MM:SS");
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

bool Transaction::isValid() const {
    return accountId_ > 0 && categoryId_ > 0 && 
           (type_ == "income" || type_ == "expense") && 
           amount_ > 0.0;
}

} // namespace accounting
