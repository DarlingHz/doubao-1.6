#include "models/Category.h"

namespace accounting {

Category::Category(int id, const std::string& name, const std::string& type)
    : id_(id), name_(name), type_(type) {
}

int Category::getId() const {
    return id_;
}

void Category::setId(int id) {
    id_ = id;
}

const std::string& Category::getName() const {
    return name_;
}

void Category::setName(const std::string& name) {
    name_ = name;
}

const std::string& Category::getType() const {
    return type_;
}

void Category::setType(const std::string& type) {
    type_ = type;
}

bool Category::isValid() const {
    return !name_.empty() && (type_ == "income" || type_ == "expense");
}

} // namespace accounting
