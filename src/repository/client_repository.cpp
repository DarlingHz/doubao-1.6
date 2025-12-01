#include "repository/client_repository.hpp"
#include "repository/db_connection_pool.hpp"
#include "utils/utils.hpp"
#include <stdexcept>
#include <iostream>

namespace api_quota {
namespace repository {

class ClientRepository::SQLiteImpl {
public:
    explicit SQLiteImpl(const std::string& db_path) : connection_pool_(db_path, 5) {
        connection_pool_.initialize();
    }
    
    ~SQLiteImpl() {
        // 连接池会在析构时自动关闭所有连接
    }
    
    DbConnectionPool& get_connection_pool() {
        return connection_pool_;
    }
    
private:
    DbConnectionPool connection_pool_;
};

ClientRepository::ClientRepository(const std::string& database_path)
    : database_path_(database_path) {
    impl_ = std::make_unique<SQLiteImpl>(database_path);
    init_database();
}

ClientRepository::~ClientRepository() = default;

bool ClientRepository::init_database() {
    const char* create_clients_table = R"(
        CREATE TABLE IF NOT EXISTS clients (
            client_id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            contact_email TEXT NOT NULL,
            daily_quota INTEGER NOT NULL DEFAULT 10000,
            per_minute_quota INTEGER NOT NULL DEFAULT 200,
            is_active BOOLEAN NOT NULL DEFAULT 1,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        );
        
        CREATE INDEX IF NOT EXISTS idx_clients_is_active ON clients(is_active);
        CREATE INDEX IF NOT EXISTS idx_clients_created_at ON clients(created_at);
    )";
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        connection->execute(create_clients_table);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<Client> ClientRepository::create_client(const std::string& name, const std::string& contact_email,
                                                    int64_t daily_quota, int64_t per_minute_quota) {
    const char* sql = R"(
        INSERT INTO clients (name, contact_email, daily_quota, per_minute_quota, is_active, created_at, updated_at)
        VALUES (?, ?, ?, ?, 1, ?, ?)
    )";
    
    auto connection = impl_->get_connection_pool().get_connection();
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
        return std::nullopt;
    }
    
    uint64_t now = utils::get_current_timestamp();
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, contact_email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, daily_quota);
    sqlite3_bind_int64(stmt, 4, per_minute_quota);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int64(stmt, 6, now);
    
    rc = sqlite3_step(stmt);
    
    uint64_t client_id = 0;
    if (rc == SQLITE_DONE) {
        client_id = sqlite3_last_insert_rowid(connection->get_db());
    } else {
        std::cerr << "Failed to insert client: " << sqlite3_errmsg(connection->get_db()) << std::endl;
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    
    sqlite3_finalize(stmt);
    
    // 返回创建的客户端
    return get_client_by_id(client_id);
}

std::vector<Client> ClientRepository::get_all_clients(bool include_inactive) {
    std::vector<Client> clients;
    const char* sql = include_inactive ?
        "SELECT client_id, name, contact_email, daily_quota, per_minute_quota, is_active, created_at, updated_at FROM clients ORDER BY created_at DESC" :
        "SELECT client_id, name, contact_email, daily_quota, per_minute_quota, is_active, created_at, updated_at FROM clients WHERE is_active = 1 ORDER BY created_at DESC";
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return clients;
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Client client;
            client.client_id = sqlite3_column_int64(stmt, 0);
            client.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            client.contact_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            client.daily_quota = sqlite3_column_int64(stmt, 3);
            client.per_minute_quota = sqlite3_column_int64(stmt, 4);
            client.is_active = sqlite3_column_int(stmt, 5) != 0;
            client.created_at = sqlite3_column_int64(stmt, 6);
            client.updated_at = sqlite3_column_int64(stmt, 7);
            
            clients.push_back(client);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting all clients: " << e.what() << std::endl;
    }
    
    return clients;
}

std::optional<Client> ClientRepository::get_client_by_id(uint64_t client_id) {
    const char* sql = R"(
        SELECT client_id, name, contact_email, daily_quota, per_minute_quota, is_active, created_at, updated_at
        FROM clients WHERE client_id = ?
    )";
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            Client client;
            client.client_id = sqlite3_column_int64(stmt, 0);
            client.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            client.contact_email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            client.daily_quota = sqlite3_column_int64(stmt, 3);
            client.per_minute_quota = sqlite3_column_int64(stmt, 4);
            client.is_active = sqlite3_column_int(stmt, 5) != 0;
            client.created_at = sqlite3_column_int64(stmt, 6);
            client.updated_at = sqlite3_column_int64(stmt, 7);
            
            sqlite3_finalize(stmt);
            return client;
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting client by ID: " << e.what() << std::endl;
    }
    
    return std::nullopt;
}

bool ClientRepository::update_client(uint64_t client_id, const std::optional<std::string>& name,
                                    const std::optional<std::string>& contact_email,
                                    const std::optional<int64_t>& daily_quota,
                                    const std::optional<int64_t>& per_minute_quota,
                                    const std::optional<bool>& is_active) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        // 构建动态SQL
        std::string sql = "UPDATE clients SET updated_at = ?";
        
        if (name.has_value()) {
            sql += ", name = ?";
        }
        if (contact_email.has_value()) {
            sql += ", contact_email = ?";
        }
        if (daily_quota.has_value()) {
            sql += ", daily_quota = ?";
        }
        if (per_minute_quota.has_value()) {
            sql += ", per_minute_quota = ?";
        }
        if (is_active.has_value()) {
            sql += ", is_active = ?";
        }
        
        sql += " WHERE client_id = ?";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        uint64_t now = utils::get_current_timestamp();
        int param_index = 1;
        
        sqlite3_bind_int64(stmt, param_index++, now);
        
        if (name.has_value()) {
            sqlite3_bind_text(stmt, param_index++, name.value().c_str(), -1, SQLITE_TRANSIENT);
        }
        if (contact_email.has_value()) {
            sqlite3_bind_text(stmt, param_index++, contact_email.value().c_str(), -1, SQLITE_TRANSIENT);
        }
        if (daily_quota.has_value()) {
            sqlite3_bind_int64(stmt, param_index++, daily_quota.value());
        }
        if (per_minute_quota.has_value()) {
            sqlite3_bind_int64(stmt, param_index++, per_minute_quota.value());
        }
        if (is_active.has_value()) {
            sqlite3_bind_int(stmt, param_index++, is_active.value() ? 1 : 0);
        }
        
        sqlite3_bind_int64(stmt, param_index, client_id);
        
        rc = sqlite3_step(stmt);
        int changes = sqlite3_changes(connection->get_db());
        
        sqlite3_finalize(stmt);
        
        return (rc == SQLITE_DONE && changes > 0);
    } catch (const std::exception& e) {
        std::cerr << "Error updating client: " << e.what() << std::endl;
        return false;
    }
}
    


bool ClientRepository::delete_client(uint64_t client_id) {
    return update_client(client_id, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::optional<bool>(false));
}

bool ClientRepository::exists(uint64_t client_id) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = "SELECT 1 FROM clients WHERE client_id = ?";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
        
        sqlite3_finalize(stmt);
        return exists;
    } catch (const std::exception& e) {
        std::cerr << "Error checking client existence: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::pair<uint64_t, int64_t>> ClientRepository::get_top_clients_by_daily_calls(int limit) {
    std::vector<std::pair<uint64_t, int64_t>> result;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT client_id, COUNT(*) as call_count
            FROM api_calls
            WHERE timestamp >= ?
            GROUP BY client_id
            ORDER BY call_count DESC
            LIMIT ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return result;
        }
        
        uint64_t today_start = utils::get_today_start_timestamp();
        sqlite3_bind_int64(stmt, 1, today_start);
        sqlite3_bind_int(stmt, 2, limit);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t client_id = sqlite3_column_int64(stmt, 0);
            int64_t call_count = sqlite3_column_int64(stmt, 1);
            result.emplace_back(client_id, call_count);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting top clients by daily calls: " << e.what() << std::endl;
    }
    
    return result;
}

std::optional<ClientRepository::ClientCallSummary> 
ClientRepository::get_client_call_summary(uint64_t client_id, uint64_t start_time, uint64_t end_time) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        ClientCallSummary summary;
        
        // 获取总调用次数
        const char* total_calls_sql = R"(
            SELECT COUNT(*) FROM api_calls
            WHERE client_id = ? AND timestamp >= ? AND timestamp <= ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), total_calls_sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            summary.total_calls = sqlite3_column_int64(stmt, 0);
        }
        
        sqlite3_finalize(stmt);
        
        // 获取拒绝调用统计
        const char* rejected_sql = R"(
            SELECT reason, COUNT(*) as count
            FROM api_calls
            WHERE client_id = ? AND timestamp >= ? AND timestamp <= ? AND allowed = 0
            GROUP BY reason
        )";
        
        rc = sqlite3_prepare_v2(connection->get_db(), rejected_sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return std::nullopt;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int64_t count = sqlite3_column_int64(stmt, 1);
            summary.rejection_reasons[reason] = count;
            summary.rejected_calls += count;
        }
        
        sqlite3_finalize(stmt);
        
        return summary;
    } catch (const std::exception& e) {
        std::cerr << "Error getting client call summary: " << e.what() << std::endl;
        return std::nullopt;
    }
}

} // namespace repository
} // namespace api_quota