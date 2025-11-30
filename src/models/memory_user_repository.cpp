#include "memory_user_repository.h"
#include <chrono>

bool MemoryUserRepository::CreateUser(User& user) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 检查用户名是否已存在
    if (username_to_id_.find(user.GetUsername()) != username_to_id_.end()) {
        return false;
    }

    // 设置用户ID和时间戳
    user.SetId(next_id_++);
    auto now = std::chrono::system_clock::now();
    user.SetCreatedAt(now);
    user.SetUpdatedAt(now);

    // 保存用户
    auto user_ptr = std::make_shared<User>(user);
    users_[user.GetId()] = user_ptr;
    username_to_id_[user.GetUsername()] = user.GetId();

    return true;
}

std::shared_ptr<User> MemoryUserRepository::GetUserById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_.find(id);
    if (it != users_.end() && !it->second->IsDeleted()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<User> MemoryUserRepository::GetUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = username_to_id_.find(username);
    if (it != username_to_id_.end()) {
        auto user_it = users_.find(it->second);
        if (user_it != users_.end() && !user_it->second->IsDeleted()) {
            return user_it->second;
        }
    }

    return nullptr;
}

bool MemoryUserRepository::UpdateUser(const User& user) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_.find(user.GetId());
    if (it == users_.end() || it->second->IsDeleted()) {
        return false;
    }

    // 检查用户名是否已被其他用户使用
    auto username_it = username_to_id_.find(user.GetUsername());
    if (username_it != username_to_id_.end() && username_it->second != user.GetId()) {
        return false;
    }

    // 更新用户信息
    auto updated_user = std::make_shared<User>(user);
    updated_user->SetCreatedAt(it->second->GetCreatedAt());
    updated_user->SetUpdatedAt(std::chrono::system_clock::now());

    users_[user.GetId()] = updated_user;
    username_to_id_[user.GetUsername()] = user.GetId();

    return true;
}

bool MemoryUserRepository::DeleteUser(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_.find(id);
    if (it == users_.end() || it->second->IsDeleted()) {
        return false;
    }

    // 软删除用户
    it->second->SetIsDeleted(true);
    it->second->SetUpdatedAt(std::chrono::system_clock::now());

    return true;
}

bool MemoryUserRepository::IsUsernameExists(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = username_to_id_.find(username);
    if (it != username_to_id_.end()) {
        auto user_it = users_.find(it->second);
        if (user_it != users_.end() && !user_it->second->IsDeleted()) {
            return true;
        }
    }

    return false;
}
