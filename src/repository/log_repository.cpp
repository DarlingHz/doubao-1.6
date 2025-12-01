#include "repository/log_repository.hpp"
#include "repository/db_connection_pool.hpp"
#include "utils/utils.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <algorithm>

namespace api_quota {
namespace repository {

class LogRepository::SQLiteImpl {
public:
    explicit SQLiteImpl(const std::string& db_path) : connection_pool_(db_path) {
        // 连接池在构造函数中自动初始化
    }
    
    ~SQLiteImpl() = default;
    
    DbConnectionPool& get_connection_pool() {
        return connection_pool_;
    }
    
private:
    DbConnectionPool connection_pool_;
};

LogRepository::LogRepository(const std::string& database_path)
    : database_path_(database_path) {
    impl_ = std::make_unique<SQLiteImpl>(database_path);
    init_database();
}

LogRepository::~LogRepository() = default;

bool LogRepository::init_database() {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* create_logs_table = R"(
            CREATE TABLE IF NOT EXISTS api_calls (
                log_id INTEGER PRIMARY KEY AUTOINCREMENT,
                client_id INTEGER NOT NULL,
                key_id INTEGER NOT NULL,
                api_key TEXT NOT NULL,
                endpoint TEXT NOT NULL,
                weight INTEGER NOT NULL DEFAULT 1,
                allowed BOOLEAN NOT NULL,
                reason TEXT,
                retry_after INTEGER NOT NULL DEFAULT 0,
                timestamp INTEGER NOT NULL,
                ip_address TEXT,
                FOREIGN KEY (client_id) REFERENCES clients(client_id),
                FOREIGN KEY (key_id) REFERENCES api_keys(key_id)
            );
            
            CREATE INDEX IF NOT EXISTS idx_api_calls_client_id ON api_calls(client_id);
            CREATE INDEX IF NOT EXISTS idx_api_calls_key_id ON api_calls(key_id);
            CREATE INDEX IF NOT EXISTS idx_api_calls_timestamp ON api_calls(timestamp);
            CREATE INDEX IF NOT EXISTS idx_api_calls_client_time ON api_calls(client_id, timestamp);
            CREATE INDEX IF NOT EXISTS idx_api_calls_key_time ON api_calls(key_id, timestamp);
        )";
        
        char* err_msg = nullptr;
        int rc = sqlite3_exec(connection->get_db(), create_logs_table, nullptr, nullptr, &err_msg);
        
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

bool LogRepository::log_api_call(uint64_t client_id, uint64_t key_id, const std::string& api_key,
                                const std::string& endpoint, int64_t weight, bool allowed,
                                const std::string& reason, uint64_t retry_after, const std::string& ip_address) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            INSERT INTO api_calls (client_id, key_id, api_key, endpoint, weight, allowed, reason, retry_after, timestamp, ip_address)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return false;
        }
        
        uint64_t now = utils::get_current_timestamp();
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, key_id);
        sqlite3_bind_text(stmt, 3, api_key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, endpoint.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, weight);
        sqlite3_bind_int(stmt, 6, allowed ? 1 : 0);
        sqlite3_bind_text(stmt, 7, reason.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 8, retry_after);
        sqlite3_bind_int64(stmt, 9, now);
        sqlite3_bind_text(stmt, 10, ip_address.c_str(), -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        bool success = (rc == SQLITE_DONE);
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error logging API call: " << e.what() << std::endl;
        return false;
    }
}

bool LogRepository::bulk_log_api_calls(const std::vector<ApiCallLog>& logs) {
    if (logs.empty()) {
        return true;
    }
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        // 开始事务以提高性能
        sqlite3_exec(connection->get_db(), "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        
        const char* sql = R"(
            INSERT INTO api_calls (client_id, key_id, api_key, endpoint, weight, allowed, reason, retry_after, timestamp, ip_address)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            sqlite3_exec(connection->get_db(), "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        bool success = true;
        
        for (const auto& log : logs) {
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
            
            sqlite3_bind_int64(stmt, 1, log.client_id);
            sqlite3_bind_int64(stmt, 2, log.key_id);
            sqlite3_bind_text(stmt, 3, log.api_key.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, log.endpoint.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 5, log.weight);
            sqlite3_bind_int(stmt, 6, log.allowed ? 1 : 0);
            sqlite3_bind_text(stmt, 7, log.reason.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 8, log.retry_after);
            sqlite3_bind_int64(stmt, 9, log.timestamp);
            sqlite3_bind_text(stmt, 10, log.ip_address.c_str(), -1, SQLITE_TRANSIENT);
            
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to insert log: " << sqlite3_errmsg(connection->get_db()) << std::endl;
                success = false;
                break;
            }
        }
        
        sqlite3_finalize(stmt);
        
        if (success) {
            sqlite3_exec(connection->get_db(), "COMMIT", nullptr, nullptr, nullptr);
        } else {
            sqlite3_exec(connection->get_db(), "ROLLBACK", nullptr, nullptr, nullptr);
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error in bulk log API calls: " << e.what() << std::endl;
        return false;
    }
}

std::vector<ApiCallLog> LogRepository::get_client_logs(uint64_t client_id, uint64_t start_time,
                                                    uint64_t end_time, int limit, int offset) {
    std::vector<ApiCallLog> logs;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT log_id, client_id, key_id, api_key, endpoint, weight, allowed, reason, retry_after, timestamp, ip_address
            FROM api_calls
            WHERE client_id = ? AND timestamp >= ? AND timestamp <= ?
            ORDER BY timestamp DESC
            LIMIT ? OFFSET ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return logs;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        sqlite3_bind_int(stmt, 4, limit);
        sqlite3_bind_int(stmt, 5, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiCallLog log;
            log.log_id = sqlite3_column_int64(stmt, 0);
            log.client_id = sqlite3_column_int64(stmt, 1);
            log.key_id = sqlite3_column_int64(stmt, 2);
            log.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            log.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            log.weight = sqlite3_column_int64(stmt, 5);
            log.allowed = sqlite3_column_int(stmt, 6) != 0;
            log.reason = sqlite3_column_text(stmt, 7) ? 
                        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
            log.retry_after = sqlite3_column_int64(stmt, 8);
            log.timestamp = sqlite3_column_int64(stmt, 9);
            log.ip_address = sqlite3_column_text(stmt, 10) ? 
                            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)) : "";
            
            logs.push_back(log);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting client logs: " << e.what() << std::endl;
    }
    
    return logs;
}

std::vector<ApiCallLog> LogRepository::get_key_logs(uint64_t key_id, uint64_t start_time,
                                                 uint64_t end_time, int limit, int offset) {
    std::vector<ApiCallLog> logs;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT log_id, client_id, key_id, api_key, endpoint, weight, allowed, reason, retry_after, timestamp, ip_address
            FROM api_calls
            WHERE key_id = ? AND timestamp >= ? AND timestamp <= ?
            ORDER BY timestamp DESC
            LIMIT ? OFFSET ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return logs;
        }
        
        sqlite3_bind_int64(stmt, 1, key_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        sqlite3_bind_int(stmt, 4, limit);
        sqlite3_bind_int(stmt, 5, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiCallLog log;
            log.log_id = sqlite3_column_int64(stmt, 0);
            log.client_id = sqlite3_column_int64(stmt, 1);
            log.key_id = sqlite3_column_int64(stmt, 2);
            log.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            log.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            log.weight = sqlite3_column_int64(stmt, 5);
            log.allowed = sqlite3_column_int(stmt, 6) != 0;
            log.reason = sqlite3_column_text(stmt, 7) ? 
                        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
            log.retry_after = sqlite3_column_int64(stmt, 8);
            log.timestamp = sqlite3_column_int64(stmt, 9);
            log.ip_address = sqlite3_column_text(stmt, 10) ? 
                            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)) : "";
            
            logs.push_back(log);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting key logs: " << e.what() << std::endl;
    }
    
    return logs;
}

LogRepository::CallStats LogRepository::get_client_call_stats(uint64_t client_id, uint64_t start_time,
                                                          uint64_t end_time) {
    CallStats stats;
    
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        const char* sql = R"(
            SELECT 
                COUNT(*) as total_calls,
                SUM(CASE WHEN allowed = 1 THEN 1 ELSE 0 END) as allowed_calls,
                SUM(CASE WHEN allowed = 0 THEN 1 ELSE 0 END) as rejected_calls,
                SUM(weight) as total_weight
            FROM api_calls
            WHERE client_id = ? AND timestamp >= ? AND timestamp <= ?
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return stats;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_calls = sqlite3_column_int64(stmt, 0);
            stats.allowed_calls = sqlite3_column_int64(stmt, 1);
            stats.rejected_calls = sqlite3_column_int64(stmt, 2);
            stats.total_weight = sqlite3_column_int64(stmt, 3);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        std::cerr << "Error getting client call stats: " << e.what() << std::endl;
    }
    
    return stats;
}

int64_t LogRepository::get_client_calls_in_time_window(uint64_t client_id, uint64_t window_start) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        int64_t count = 0;
        const char* sql = R"(
            SELECT SUM(weight) FROM api_calls
            WHERE client_id = ? AND timestamp >= ? AND allowed = 1
        )";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->get_db()) << std::endl;
            return 0;
        }
        
        sqlite3_bind_int64(stmt, 1, client_id);
        sqlite3_bind_int64(stmt, 2, window_start);
        
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            count = sqlite3_column_int64(stmt, 0);
        }
        
        sqlite3_finalize(stmt);
        return count;
    } catch (const std::exception& e) {
        std::cerr << "Error getting client calls in time window: " << e.what() << std::endl;
        return 0;
    }
}

std::unordered_map<std::string, LogRepository::EndpointStats> LogRepository::get_endpoint_stats(
    std::optional<std::string> from_date, 
    std::optional<std::string> to_date) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        std::string sql = R"(
            SELECT endpoint, COUNT(*) as total_calls, 
                   SUM(CASE WHEN allowed = 1 THEN 1 ELSE 0 END) as allowed_calls,
                   AVG(response_time) as avg_response_time
            FROM api_calls
            WHERE 1=1
        )";
        
        std::vector<std::string> params;
        
        if (from_date) {
            sql += " AND timestamp >= ?";
            params.push_back(from_date.value());
        }
        
        if (to_date) {
            sql += " AND timestamp <= ?";
            params.push_back(to_date.value());
        }
        
        sql += " GROUP BY endpoint ORDER BY total_calls DESC";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(connection->get_db(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare endpoint stats statement: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        // 绑定参数
        for (size_t i = 0; i < params.size(); i++) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        
        std::unordered_map<std::string, EndpointStats> result;
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string endpoint(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            EndpointStats stats;
            stats.total_calls = sqlite3_column_int64(stmt, 1);
            stats.allowed_calls = sqlite3_column_int64(stmt, 2);
            stats.avg_response_time = sqlite3_column_double(stmt, 3);
            result[endpoint] = stats;
        }
        
        sqlite3_finalize(stmt);
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error getting endpoint stats: " << e.what() << std::endl;
        throw;
    }
}

std::unordered_map<std::string, uint64_t> LogRepository::get_denial_reason_distribution(
    std::optional<uint64_t> client_id,
    std::optional<std::string> from_date,
    std::optional<std::string> to_date) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        std::string sql = R"(
            SELECT reason, COUNT(*) as count
            FROM api_calls
            WHERE allowed = 0
        )";
        
        std::vector<std::string> params;
        
        if (client_id) {
            sql += " AND client_id = ?";
            params.push_back(std::to_string(client_id.value()));
        }
        
        if (from_date) {
            sql += " AND timestamp >= ?";
            params.push_back(from_date.value());
        }
        
        if (to_date) {
            sql += " AND timestamp <= ?";
            params.push_back(to_date.value());
        }
        
        sql += " GROUP BY reason ORDER BY count DESC";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(connection->get_db(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare denial reasons statement: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        // 绑定参数
        for (size_t i = 0; i < params.size(); i++) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        
        std::unordered_map<std::string, uint64_t> result;
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string reason(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            uint64_t count = sqlite3_column_int64(stmt, 1);
            result[reason] = count;
        }
        
        sqlite3_finalize(stmt);
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error getting denial reason distribution: " << e.what() << std::endl;
        throw;
    }
}

std::vector<ApiCallLog> LogRepository::get_recent_logs(size_t limit) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        sqlite3_stmt* stmt;
        const char* sql = R"(
            SELECT log_id, client_id, api_key, endpoint, allowed, reason, weight, timestamp, ip_address
            FROM api_calls
            ORDER BY timestamp DESC
            LIMIT ?
        )";
        
        if (sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare recent logs statement: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        sqlite3_bind_int(stmt, 1, static_cast<int>(limit));
        
        std::vector<ApiCallLog> logs;
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ApiCallLog log;
            log.log_id = sqlite3_column_int64(stmt, 0);
            log.client_id = sqlite3_column_int64(stmt, 1);
            log.api_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            log.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            log.allowed = sqlite3_column_int(stmt, 4) != 0;
            if (sqlite3_column_text(stmt, 5)) {
                log.reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }
            log.weight = sqlite3_column_int(stmt, 6);
            log.timestamp = sqlite3_column_int64(stmt, 7);
            if (sqlite3_column_text(stmt, 8)) {
                log.ip_address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            }
            logs.push_back(log);
        }
        
        sqlite3_finalize(stmt);
        
        // 由于我们是按时间倒序查询的，现在需要反转一下，使其按时间正序排列
        std::reverse(logs.begin(), logs.end());
        
        return logs;
    } catch (const std::exception& e) {
        std::cerr << "Error getting recent logs: " << e.what() << std::endl;
        throw;
    }
}

LogRepository::DailyStats LogRepository::get_daily_stats() {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        // 获取今天的开始时间
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        std::tm local_tm;
#ifdef _WIN32
        localtime_s(&local_tm, &time_t_now);
#else
        localtime_r(&time_t_now, &local_tm);
#endif
        
        local_tm.tm_hour = 0;
        local_tm.tm_min = 0;
        local_tm.tm_sec = 0;
        
        auto today_start = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
        auto today_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            today_start.time_since_epoch()).count();
        
        sqlite3_stmt* stmt;
        const char* sql = R"(
            SELECT COUNT(*) as total_calls, 
                   SUM(CASE WHEN allowed = 1 THEN 1 ELSE 0 END) as allowed_calls
            FROM api_calls
            WHERE timestamp >= ?
        )";
        
        if (sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare daily stats statement: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        sqlite3_bind_int64(stmt, 1, today_timestamp);
        
        DailyStats stats{0, 0};
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_calls = sqlite3_column_int64(stmt, 0);
            stats.allowed_calls = sqlite3_column_int64(stmt, 1);
        }
        
        sqlite3_finalize(stmt);
        return stats;
    } catch (const std::exception& e) {
        std::cerr << "Error getting daily stats: " << e.what() << std::endl;
        throw;
    }
}

int64_t LogRepository::cleanup_old_logs(int days_to_keep) {
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        
        // 计算截止时间戳
        auto now = std::chrono::system_clock::now();
        auto cutoff_time = now - std::chrono::hours(24 * days_to_keep);
        auto cutoff_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            cutoff_time.time_since_epoch()).count();
        
        sqlite3_stmt* stmt;
        const char* sql = "DELETE FROM api_calls WHERE timestamp < ?";
        
        if (sqlite3_prepare_v2(connection->get_db(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare cleanup statement: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        sqlite3_bind_int64(stmt, 1, cutoff_timestamp);
        
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Failed to execute cleanup: " + 
                                   std::string(sqlite3_errmsg(connection->get_db())));
        }
        
        int64_t deleted_count = sqlite3_changes64(connection->get_db());
        sqlite3_finalize(stmt);
        
        // 执行VACUUM来释放空间
        sqlite3_exec(connection->get_db(), "VACUUM", nullptr, nullptr, nullptr);
        
        return deleted_count;
    } catch (const std::exception& e) {
        std::cerr << "Error cleaning up old logs: " << e.what() << std::endl;
        throw;
    }
}

LogRepository::RealtimeStats LogRepository::get_realtime_stats() {
    RealtimeStats stats;
    try {
        auto connection = impl_->get_connection_pool().get_connection();
        uint64_t now = utils::get_current_timestamp();
        uint64_t minute_ago = now - 60;
        uint64_t hour_ago = now - 3600;
        
        // 获取最近一分钟的调用数
        const char* minute_sql = "SELECT COUNT(*) FROM api_calls WHERE timestamp >= ?";
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(connection->get_db(), minute_sql, -1, &stmt, nullptr);
        
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, minute_ago);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.calls_last_minute = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        
        // 获取最近一小时的调用数
        const char* hour_sql = "SELECT COUNT(*) FROM api_calls WHERE timestamp >= ?";
        rc = sqlite3_prepare_v2(connection->get_db(), hour_sql, -1, &stmt, nullptr);
        
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, hour_ago);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.calls_last_hour = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        
        // 获取最近一分钟的拒绝数
        const char* rejected_sql = "SELECT COUNT(*) FROM api_calls WHERE timestamp >= ? AND allowed = 0";
        rc = sqlite3_prepare_v2(connection->get_db(), rejected_sql, -1, &stmt, nullptr);
        
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, minute_ago);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.rejected_last_minute = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        
        // 这里可以添加响应时间统计，如果数据库中有相关字段
    } catch (const std::exception& e) {
        std::cerr << "Error getting realtime stats: " << e.what() << std::endl;
    }
    
    return stats;
}

} // namespace repository
} // namespace api_quota