#include "models/Account.h"

namespace accounting {

Account::Account(int id, const std::string& name, const std::string& type, double initialBalance)
    : id_(id), name_(name), type_(type), initialBalance_(initialBalance), currentBalance_(initialBalance) {
}

int Account::getId() const {
    return id_;
}

void Account::setId(int id) {
    id_ = id;
}

const std::string& Account::getName() const {
    return name_;
}

void Account::setName(const std::string& name) {
    name_ = name;
}

const std::string& Account::getType() const {
    return type_;
}

void Account::setType(const std::string& type) {
    type_ = type;
}

double Account::getInitialBalance() const {
    return initialBalance_;
}

void Account::setInitialBalance(double balance) {
    initialBalance_ = balance;
}

double Account::getCurrentBalance() const {
    return currentBalance_;
}

void Account::setCurrentBalance(double balance) {
    currentBalance_ = balance;
}

bool Account::isValid() const {
    return !name_.empty() && !type_.empty() && initialBalance_ >= 0.0;
}

} // namespace accounting
