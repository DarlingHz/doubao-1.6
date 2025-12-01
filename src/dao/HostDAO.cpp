#include <dao/HostDAO.h>
#include <util/Logger.h>
#include <ctime>

namespace dao {

HostDAO::HostDAO(std::shared_ptr<DbConnectionPool> pool)
    : pool_(pool) {
}

bool HostDAO::createTable() {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS hosts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                ip TEXT NOT NULL,
                environment TEXT NOT NULL,
                tags TEXT,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL,
                is_deleted INTEGER DEFAULT 0
            );
            
            CREATE INDEX IF NOT EXISTS idx_hosts_environment ON hosts(environment, is_deleted);
            CREATE INDEX IF NOT EXISTS idx_hosts_name ON hosts(name, is_deleted);
        )";
        
        char* errmsg = nullptr;
        int result = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
        if (result != SQLITE_OK) {
            LOG_ERROR("创建hosts表失败: " + std::string(errmsg));
            sqlite3_free(errmsg);
            return false;
        }
        
        LOG_INFO("hosts表创建成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("创建hosts表异常: " + std::string(e.what()));
        return false;
    }
}

int HostDAO::create(const model::Host& host) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        auto now = std::chrono::system_clock::now();
        auto now_str = std::to_string(std::chrono::system_clock::to_time_t(now));
        
        const char* sql = "INSERT INTO hosts (name, ip, environment, tags, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return 0;
        }
        
        sqlite3_bind_text(stmt, 1, host.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, host.ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, host.environment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, host.tags.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, now_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, now_str.c_str(), -1, SQLITE_TRANSIENT);
        
        result = sqlite3_step(stmt);
        int id = 0;
        if (result == SQLITE_DONE) {
            id = sqlite3_last_insert_rowid(db);
            LOG_INFO("创建主机成功，ID: " + std::to_string(id));
        } else {
            LOG_ERROR("执行SQL失败: " + std::string(sqlite3_errmsg(db)));
        }
        
        sqlite3_finalize(stmt);
        return id;
    } catch (const std::exception& e) {
        LOG_ERROR("创建主机异常: " + std::string(e.what()));
        return 0;
    }
}

std::optional<model::Host> HostDAO::getById(int id) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, name, ip, environment, tags, created_at, updated_at, is_deleted FROM hosts WHERE id = ? AND is_deleted = 0";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return std::nullopt;
        }
        
        sqlite3_bind_int(stmt, 1, id);
        
        model::Host host;
        bool found = false;
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            host.id = sqlite3_column_int(stmt, 0);
            host.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            host.ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            host.environment = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (sqlite3_column_text(stmt, 4)) {
                host.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            }
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            time_t updated_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            host.created_at = std::chrono::system_clock::from_time_t(created_at);
            host.updated_at = std::chrono::system_clock::from_time_t(updated_at);
            host.is_deleted = sqlite3_column_int(stmt, 7) != 0;
            
            found = true;
        }
        
        sqlite3_finalize(stmt);
        
        if (found) {
            return host;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("查询主机异常: " + std::string(e.what()));
        return std::nullopt;
    }
}

std::vector<model::Host> HostDAO::getAll(int limit, int offset) {
    std::vector<model::Host> hosts;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, name, ip, environment, tags, created_at, updated_at, is_deleted FROM hosts WHERE is_deleted = 0 LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return hosts;
        }
        
        sqlite3_bind_int(stmt, 1, limit);
        sqlite3_bind_int(stmt, 2, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Host host;
            host.id = sqlite3_column_int(stmt, 0);
            host.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            host.ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            host.environment = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (sqlite3_column_text(stmt, 4)) {
                host.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            }
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            time_t updated_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            host.created_at = std::chrono::system_clock::from_time_t(created_at);
            host.updated_at = std::chrono::system_clock::from_time_t(updated_at);
            host.is_deleted = sqlite3_column_int(stmt, 7) != 0;
            
            hosts.push_back(host);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("查询所有主机异常: " + std::string(e.what()));
    }
    return hosts;
}

std::vector<model::Host> HostDAO::getByEnvironment(const std::string& environment, int limit, int offset) {
    std::vector<model::Host> hosts;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, name, ip, environment, tags, created_at, updated_at, is_deleted FROM hosts WHERE environment = ? AND is_deleted = 0 LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return hosts;
        }
        
        sqlite3_bind_text(stmt, 1, environment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, limit);
        sqlite3_bind_int(stmt, 3, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Host host;
            // 与getAll相同的解析逻辑
            host.id = sqlite3_column_int(stmt, 0);
            host.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            host.ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            host.environment = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (sqlite3_column_text(stmt, 4)) {
                host.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            }
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            time_t updated_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            host.created_at = std::chrono::system_clock::from_time_t(created_at);
            host.updated_at = std::chrono::system_clock::from_time_t(updated_at);
            host.is_deleted = sqlite3_column_int(stmt, 7) != 0;
            
            hosts.push_back(host);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("按环境查询主机异常: " + std::string(e.what()));
    }
    return hosts;
}

std::vector<model::Host> HostDAO::search(const std::string& keyword, int limit, int offset) {
    std::vector<model::Host> hosts;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, name, ip, environment, tags, created_at, updated_at, is_deleted FROM hosts WHERE (name LIKE ? OR ip LIKE ? OR environment LIKE ? OR tags LIKE ?) AND is_deleted = 0 LIMIT ? OFFSET ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return hosts;
        }
        
        std::string like_pattern = "%" + keyword + "%";
        for (int i = 1; i <= 4; ++i) {
            sqlite3_bind_text(stmt, i, like_pattern.c_str(), -1, SQLITE_TRANSIENT);
        }
        sqlite3_bind_int(stmt, 5, limit);
        sqlite3_bind_int(stmt, 6, offset);
        
        // 解析逻辑与getAll相同
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Host host;
            host.id = sqlite3_column_int(stmt, 0);
            host.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            host.ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            host.environment = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            if (sqlite3_column_text(stmt, 4)) {
                host.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            }
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            time_t updated_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            host.created_at = std::chrono::system_clock::from_time_t(created_at);
            host.updated_at = std::chrono::system_clock::from_time_t(updated_at);
            host.is_deleted = sqlite3_column_int(stmt, 7) != 0;
            
            hosts.push_back(host);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("搜索主机异常: " + std::string(e.what()));
    }
    return hosts;
}

bool HostDAO::update(const model::Host& host) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        auto now = std::chrono::system_clock::now();
        auto now_str = std::to_string(std::chrono::system_clock::to_time_t(now));
        
        const char* sql = "UPDATE hosts SET name = ?, ip = ?, environment = ?, tags = ?, updated_at = ? WHERE id = ? AND is_deleted = 0";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, host.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, host.ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, host.environment.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, host.tags.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, now_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, host.id);
        
        result = sqlite3_step(stmt);
        bool success = (result == SQLITE_DONE && sqlite3_changes(db) > 0);
        
        if (success) {
            LOG_INFO("更新主机成功，ID: " + std::to_string(host.id));
        } else {
            LOG_ERROR("更新主机失败，ID: " + std::to_string(host.id));
        }
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("更新主机异常: " + std::string(e.what()));
        return false;
    }
}

bool HostDAO::deleteById(int id) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "UPDATE hosts SET is_deleted = 1 WHERE id = ? AND is_deleted = 0";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return false;
        }
        
        sqlite3_bind_int(stmt, 1, id);
        
        result = sqlite3_step(stmt);
        bool success = (result == SQLITE_DONE && sqlite3_changes(db) > 0);
        
        if (success) {
            LOG_INFO("删除主机成功，ID: " + std::to_string(id));
        } else {
            LOG_ERROR("删除主机失败，ID: " + std::to_string(id));
        }
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("删除主机异常: " + std::string(e.what()));
        return false;
    }
}

} // namespace dao
