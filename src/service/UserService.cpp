#include "service/UserService.h"
#include <stdexcept>
#include <regex>

namespace service {

UserService::UserService(std::shared_ptr<repository::UserRepository> repository)
    : repository_(repository) {}

std::optional<model::User> UserService::createUser(const std::string& nickname) {
    // 验证昵称
    if (!validateNickname(nickname)) {
        throw std::invalid_argument("昵称格式不正确，长度应为1-50个字符");
    }
    
    // 检查昵称是否已存在
    if (repository_->findByNickname(nickname).has_value()) {
        throw std::invalid_argument("昵称已存在");
    }
    
    // 创建用户
    model::User user;
    user.setNickname(nickname);
    
    return repository_->create(user);
}

std::optional<model::User> UserService::getUserById(int id) {
    if (id <= 0) {
        throw std::invalid_argument("无效的用户ID");
    }
    
    return repository_->findById(id);
}

bool UserService::validateUser(const model::User& user) {
    return validateNickname(user.getNickname());
}

bool UserService::validateNickname(const std::string& nickname) {
    // 昵称长度1-50个字符，只允许中文、英文、数字、下划线
    if (nickname.empty() || nickname.length() > 50) {
        return false;
    }
    
    std::regex pattern("^[\\u4e00-\\u9fa5a-zA-Z0-9_]+$");
    return std::regex_match(nickname, pattern);
}

} // namespace service
