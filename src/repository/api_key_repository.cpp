#include "repository/api_key_repository.hpp"
#include "repository/db_connection_pool.hpp"
#include "utils/utils.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <optional>
#include <memory>

namespace api_quota {
namespace repository {

// 实现ApiKey::get_status()方法
ApiKeyStatus ApiKey::get_status() const {
    if (is_revoked) {
        return ApiKeyStatus::REVOKED;
    }
    if (expires_at > 0 && expires_at < utils::get_current_timestamp()) {
        return ApiKeyStatus::EXPIRED;
    }
    return ApiKeyStatus::ACTIVE;
}

class ApiKeyRepository::SQLiteImpl {
public:
    explicit SQLiteImpl(const std::string& db_path) : connection_pool_(db_path) {
    }
    
    ~SQLiteImpl() = default;
    
    DbConnectionPool& get_connection_pool() {
        return connection_pool_;
    }
    
private:
    DbConnectionPool connection_pool_;
};

ApiKeyRepository::ApiKeyRepository(const std::string& database_path)
    : database_path_(database_path) {
    impl_ = std::make_unique<SQLiteImpl>(database_path);
    init_database();
}

ApiKeyRepository::~ApiKeyRepository() = default;

bool ApiKeyRepository::init_database() {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* create_api_keys_table = R"(
            CREATE TABLE IF NOT EXISTS api_keys (
                key_id INTEGER PRIMARY KEY AUTOINCREMENT,
                client_id INTEGER NOT NULL,
                api_key TEXT NOT NULL UNIQUE,
                description TEXT,
                expires_at INTEGER NOT NULL DEFAULT 0,
                is_revoked BOOLEAN NOT NULL DEFAULT 0,
                created_at INTEGER NOT NULL,
                updated_at INTEGER NOT NULL,
                FOREIGN KEY (client_id) REFERENCES clients(client_id)
            );
            
            CREATE INDEX IF NOT EXISTS idx_api_keys_client_id ON api_keys(client_id);
            CREATE INDEX IF NOT EXISTS idx_api_keys_api_key ON api_keys(api_key);
            CREATE INDEX IF NOT EXISTS idx_api_keys_expires_at ON api_keys(expires_at);
        )";
        
        char* err_msg = nullptr;
        int rc = sqlite3_exec(connection->get_db(), create_api_keys_table, nullptr, nullptr, &err_msg);
        
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << err_msg << std::endl;
            sqlite3_free(err_msg);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing database: " << e.what() << std::endl;
        return false;
    }
}

std::optional<ApiKey> ApiKeyRepository::create_api_key(uint64_t client_id, const std::string& api_key,
                                                    const std::string& description, uint64_t expires_at) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            INSERT INTO api_keys (client_id, api_key, description, expires_at, is_revoked, created_at, updated_at)
            VALUES (?, ?, ?, ?, 0, ?, ?)
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        uint64_t now = utils::get_current_timestamp();
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_text(stmt, 2, api_key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 4, expires_at);
        sqlite3_bind_int64(stmt, 5, now);
        sqlite3_bind_int64(stmt, 6, now);
        
        rc = sqlite3_step(stmt);
        
        uint64_t key_id = 0;
        if (rc == SQLITE_DONE) {
            key_id = sqlite3_last_insert_rowid(connection->get_db());
        } else {
            std::cerr << "Failed to insert API key: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            sqlite3_finalize(stmt);
            return std::nullopt;
        }
        
        sqlite3_finalize(stmt);
        
        // 返回创建的API Key
        return get_api_key_by_id(key_id);
    } catch (const std::exception& e) {
        std::cerr << "Error creating API key: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<ApiKey> ApiKeyRepository::get_api_keys_by_client_id(uint64_t client_id) {
    std::vector<ApiKey> keys;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT key_id, client_id, api_key, description, expires_at, is_revoked, created_at, updated_at
            FROM api_keys WHERE client_id = ? ORDER BY created_at DESC
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return keys;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiKey key;
            key.key_id = sqlite3_column_int64(stmt, 0);
            key.client_id = sqlite3_column_int64(stmt, 1);
            key.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            key.description = sqlite3_column_text(stmt, 3) ? 
                             reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
            key.expires_at = sqlite3_column_int64(stmt, 4);
            key.is_revoked = sqlite3_column_int(stmt, 5) != 0;
            key.created_at = sqlite3_column_int64(stmt, 6);
            key.updated_at = sqlite3_column_int64(stmt, 7);
            
            keys.push_back(key);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting API keys by client ID: " << e.what() << std::endl;
    }
    
    return keys;
}

std::optional<ApiKey> ApiKeyRepository::get_api_key_by_key(const std::string& api_key) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT key_id, client_id, api_key, description, expires_at, is_revoked, created_at, updated_at
            FROM api_keys WHERE api_key = ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_text(stmt, 1, api_key.c_str(), -1, SQLITE_TRANSIENT);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiKey key;
            key.key_id = sqlite3_column_int64(stmt, 0);
            key.client_id = sqlite3_column_int64(stmt, 1);
            key.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            key.description = sqlite3_column_text(stmt, 3) ? 
                             reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
            key.expires_at = sqlite3_column_int64(stmt, 4);
            key.is_revoked = sqlite3_column_int(stmt, 5) != 0;
            key.created_at = sqlite3_column_int64(stmt, 6);
            key.updated_at = sqlite3_column_int64(stmt, 7);
            
            sqlite3_finalize(stmt);
            return key;
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting API key by key string: " << e.what() << std::endl;
    }
    
    return std::nullopt;
}

std::optional<ApiKey> ApiKeyRepository::get_api_key_by_id(uint64_t key_id) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT key_id, client_id, api_key, description, expires_at, is_revoked, created_at, updated_at
            FROM api_keys WHERE key_id = ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiKey key;
            key.key_id = sqlite3_column_int64(stmt, 0);
            key.client_id = sqlite3_column_int64(stmt, 1);
            key.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            key.description = sqlite3_column_text(stmt, 3) ? 
                             reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)) : "";
            key.expires_at = sqlite3_column_int64(stmt, 4);
            key.is_revoked = sqlite3_column_int(stmt, 5) != 0;
            key.created_at = sqlite3_column_int64(stmt, 6);
            key.updated_at = sqlite3_column_int64(stmt, 7);
            
            sqlite3_finalize(stmt);
            return key;
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting API key by ID: " << e.what() << std::endl;
    }
    
    return std::nullopt;
}

bool ApiKeyRepository::revoke_api_key(uint64_t key_id) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            UPDATE api_keys SET is_revoked = 1, updated_at = ? WHERE key_id = ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        uint64_t now = utils::get_current_timestamp();
        sqlite3_bind_int64(stmt, 1, now);
        sqlite3_bind_int64(stmt, 2, key_id);
        
        rc = sqlite3_step(stmt);
        bool success = (rc == SQLITE_DONE && sqlite3_changes(connection->get_db()) > 0);
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error revoking API key: " << e.what() << std::endl;
        return false;
    }
}

bool ApiKeyRepository::revoke_all_client_keys(uint64_t client_id) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            UPDATE api_keys SET is_revoked = 1, updated_at = ? WHERE client_id = ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        uint64_t now = utils::get_current_timestamp();
        sqlite3_bind_int64(stmt, 1, now);
        sqlite3_bind_int64(stmt, 2, client_id);
        
        rc = sqlite3_step(stmt);
        bool success = (rc == SQLITE_DONE && sqlite3_changes(connection->get_db()) > 0);
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error revoking all client keys: " << e.what() << std::endl;
        return false;
    }
}

bool ApiKeyRepository::is_key_valid(const std::string& api_key) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT COUNT(*) FROM api_keys 
            WHERE api_key = ? AND is_revoked = 0 
            AND (expires_at = 0 OR expires_at > ?)
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, api_key.c_str(), -1, SQLITE_TRANSIENT);
        int64_t now = utils::get_current_timestamp();
        sqlite3_bind_int64(stmt, 2, now);
        
        bool valid = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            valid = sqlite3_column_int(stmt, 0) > 0;
        }
        
        sqlite3_finalize(stmt);
        return valid;
    } catch (const std::exception& e) {
        std::cerr << "Error checking key validity: " << e.what() << std::endl;
        return false;
    }
}

std::optional<ApiKeyRepository::KeyUsageStats> ApiKeyRepository::get_key_usage_stats(uint64_t key_id) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        KeyUsageStats stats;
        uint64_t now = utils::get_current_timestamp();
        uint64_t today_start = utils::get_today_start_timestamp();
        uint64_t minute_start = utils::get_current_minute_start_timestamp();
        
        // 获取总调用次数
        const char* total_sql = R"(
            SELECT COUNT(*) FROM api_calls WHERE key_id = ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), total_sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_calls = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        // 获取今日调用次数
        const char* today_sql = R"(
            SELECT COUNT(*) FROM api_calls WHERE key_id = ? AND timestamp >= ?
        )";
        
        rc = sqlite3_prepare_v2(connection->get_db(), today_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        sqlite3_bind_int64(stmt, 2, today_start);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.today_calls = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        // 获取当前分钟调用次数
        const char* minute_sql = R"(
            SELECT COUNT(*) FROM api_calls WHERE key_id = ? AND timestamp >= ?
        )";
        
        rc = sqlite3_prepare_v2(connection->get_db(), minute_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        sqlite3_bind_int64(stmt, 2, minute_start);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.current_minute_calls = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        // 获取最后使用时间
        const char* last_used_sql = R"(
            SELECT MAX(timestamp) FROM api_calls WHERE key_id = ?
        )";
        
        rc = sqlite3_prepare_v2(connection->get_db(), last_used_sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            stats.last_used_at = sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
        
        return stats;
    } catch (const std::exception& e) {
        std::cerr << "Error getting key usage stats: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<std::pair<uint64_t, int64_t>> ApiKeyRepository::get_key_timeline(uint64_t key_id, int hours_back) {
    std::vector<std::pair<uint64_t, int64_t>> timeline;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        uint64_t now = utils::get_current_timestamp();
        uint64_t start_time = now - (hours_back * 3600);
        
        const char* sql = R"(
            SELECT 
                (timestamp / 3600) * 3600 as hour,
                COUNT(*) as call_count
            FROM api_calls
            WHERE key_id = ? AND timestamp >= ? AND timestamp <= ?
            GROUP BY hour
            ORDER BY hour ASC
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return timeline;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, now);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t hour = sqlite3_column_int64(stmt, 0);
            int64_t call_count = sqlite3_column_int64(stmt, 1);
            timeline.emplace_back(hour, call_count);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting key timeline: " << e.what() << std::endl;
    }
    
    return timeline;
}

} // namespace repository
} // namespace api_quota