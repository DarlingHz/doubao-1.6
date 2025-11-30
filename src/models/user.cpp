#include "user.h"

int User::GetId() const {
    return id_;
}

void User::SetId(int id) {
    id_ = id;
}

std::string User::GetUsername() const {
    return username_;
}

void User::SetUsername(const std::string& username) {
    username_ = username;
}

std::string User::GetPasswordHash() const {
    return password_hash_;
}

void User::SetPasswordHash(const std::string& password_hash) {
    password_hash_ = password_hash;
}

std::chrono::system_clock::time_point User::GetCreatedAt() const {
    return created_at_;
}

void User::SetCreatedAt(const std::chrono::system_clock::time_point& created_at) {
    created_at_ = created_at;
}

std::chrono::system_clock::time_point User::GetUpdatedAt() const {
    return updated_at_;
}

void User::SetUpdatedAt(const std::chrono::system_clock::time_point& updated_at) {
    updated_at_ = updated_at;
}

bool User::IsDeleted() const {
    return is_deleted_;
}

void User::SetIsDeleted(bool is_deleted) {
    is_deleted_ = is_deleted;
}
