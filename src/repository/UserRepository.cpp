#include "repository/UserRepository.h"
#include <sstream>
#include <iomanip>

namespace repository {

UserRepository::UserRepository(DatabasePtr db) : db_(db) {}

std::optional<model::User> UserRepository::create(const model::User& user) {
    std::ostringstream sql;
    sql << "INSERT INTO users (nickname) VALUES ('" << user.getNickname() << "');";
    
    if (db_->execute(sql.str())) {
        int id = static_cast<int>(db_->getLastInsertId());
        return findById(id);
    }
    
    return std::nullopt;
}

std::optional<model::User> UserRepository::findById(int id) {
    std::ostringstream sql;
    sql << "SELECT id, nickname, created_at, updated_at FROM users WHERE id = " << id << ";";
    
    std::optional<model::User> result;
    
    auto callback = [](void* data, int argc, char** argv, char**) -> int {
        auto* result = static_cast<std::optional<model::User>*>(data);
        if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3]) {
            *result = UserRepository::buildUserFromRow(argv);
        }
        return 0;
    };
    
    db_->query(sql.str(), callback, &result);
    return result;
}

std::vector<model::User> UserRepository::findAll() {
    std::vector<model::User> users;
    
    auto callback = [](void* data, int argc, char** argv, char**) -> int {
        auto* users = static_cast<std::vector<model::User>*>(data);
        if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3]) {
            users->push_back(UserRepository::buildUserFromRow(argv));
        }
        return 0;
    };
    
    db_->query("SELECT id, nickname, created_at, updated_at FROM users;", callback, &users);
    return users;
}

std::optional<model::User> UserRepository::findByNickname(const std::string& nickname) {
    std::ostringstream sql;
    sql << "SELECT id, nickname, created_at, updated_at FROM users WHERE nickname = '" << nickname << "';";
    
    std::optional<model::User> result;
    
    auto callback = [](void* data, int argc, char** argv, char**) -> int {
        auto* result = static_cast<std::optional<model::User>*>(data);
        if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3]) {
            *result = UserRepository::buildUserFromRow(argv);
        }
        return 0;
    };
    
    db_->query(sql.str(), callback, &result);
    return result;
}

bool UserRepository::update(const model::User& user) {
    std::ostringstream sql;
    sql << "UPDATE users SET nickname = '" << user.getNickname() << "', updated_at = CURRENT_TIMESTAMP WHERE id = " << user.getId() << ";";
    return db_->execute(sql.str());
}

model::User UserRepository::buildUserFromRow(char** row) {
    int id = std::stoi(row[0]);
    std::string nickname = row[1];
    
    // 解析时间字符串
    std::tm tm = {};
    std::istringstream created_at_ss(row[2]);
    created_at_ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::time_t created_at = std::mktime(&tm);
    
    std::tm tm2 = {};
    std::istringstream updated_at_ss(row[3]);
    updated_at_ss >> std::get_time(&tm2, "%Y-%m-%d %H:%M:%S");
    std::time_t updated_at = std::mktime(&tm2);
    
    return model::User(id, nickname, created_at, updated_at);
}

} // namespace repository
