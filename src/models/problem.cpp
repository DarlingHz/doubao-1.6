#include "problem.h"

int Problem::GetId() const {
    return id_;
}

void Problem::SetId(int id) {
    id_ = id;
}

std::string Problem::GetTitle() const {
    return title_;
}

void Problem::SetTitle(const std::string& title) {
    title_ = title;
}

std::string Problem::GetDescription() const {
    return description_;
}

void Problem::SetDescription(const std::string& description) {
    description_ = description;
}

Difficulty Problem::GetDifficulty() const {
    return difficulty_;
}

void Problem::SetDifficulty(Difficulty difficulty) {
    difficulty_ = difficulty;
}

std::vector<std::string> Problem::GetTags() const {
    return tags_;
}

void Problem::SetTags(const std::vector<std::string>& tags) {
    tags_ = tags;
}

std::chrono::system_clock::time_point Problem::GetCreatedAt() const {
    return created_at_;
}

void Problem::SetCreatedAt(const std::chrono::system_clock::time_point& created_at) {
    created_at_ = created_at;
}

std::chrono::system_clock::time_point Problem::GetUpdatedAt() const {
    return updated_at_;
}

void Problem::SetUpdatedAt(const std::chrono::system_clock::time_point& updated_at) {
    updated_at_ = updated_at;
}

bool Problem::IsDeleted() const {
    return is_deleted_;
}

void Problem::SetIsDeleted(bool is_deleted) {
    is_deleted_ = is_deleted;
}
