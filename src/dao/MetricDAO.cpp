#include <dao/MetricDAO.h>
#include <util/Logger.h>

namespace dao {

MetricDAO::MetricDAO(std::shared_ptr<DbConnectionPool> pool)
    : pool_(pool) {
}

bool MetricDAO::createTable() {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS metrics (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                host_id INTEGER NOT NULL,
                timestamp TEXT NOT NULL,
                cpu_usage REAL NOT NULL,
                mem_usage REAL NOT NULL,
                disk_usage REAL NOT NULL,
                FOREIGN KEY (host_id) REFERENCES hosts(id) ON DELETE CASCADE
            );
            
            CREATE INDEX IF NOT EXISTS idx_metrics_host_id_timestamp ON metrics(host_id, timestamp);
            CREATE INDEX IF NOT EXISTS idx_metrics_timestamp ON metrics(timestamp);
        )";
        
        char* errmsg = nullptr;
        int result = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
        if (result != SQLITE_OK) {
            LOG_ERROR("创建metrics表失败: " + std::string(errmsg));
            sqlite3_free(errmsg);
            return false;
        }
        
        LOG_INFO("metrics表创建成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("创建metrics表异常: " + std::string(e.what()));
        return false;
    }
}

bool MetricDAO::batchInsert(const std::vector<model::Metric>& metrics) {
    if (metrics.empty()) {
        return true;
    }
    
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        // 开始事务
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        
        const char* sql = "INSERT INTO metrics (host_id, timestamp, cpu_usage, mem_usage, disk_usage) VALUES (?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        bool all_success = true;
        for (const auto& metric : metrics) {
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
            
            sqlite3_bind_int(stmt, 1, metric.host_id);
            
            auto timestamp_str = std::to_string(std::chrono::system_clock::to_time_t(metric.timestamp));
            sqlite3_bind_text(stmt, 2, timestamp_str.c_str(), -1, SQLITE_TRANSIENT);
            
            sqlite3_bind_double(stmt, 3, metric.cpu_usage);
            sqlite3_bind_double(stmt, 4, metric.mem_usage);
            sqlite3_bind_double(stmt, 5, metric.disk_usage);
            
            result = sqlite3_step(stmt);
            if (result != SQLITE_DONE) {
                LOG_ERROR("执行SQL失败: " + std::string(sqlite3_errmsg(db)));
                all_success = false;
                break;
            }
        }
        
        sqlite3_finalize(stmt);
        
        if (all_success) {
            sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
            LOG_INFO("批量插入指标数据成功，条数: " + std::to_string(metrics.size()));
        } else {
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        }
        
        return all_success;
    } catch (const std::exception& e) {
        LOG_ERROR("批量插入指标数据异常: " + std::string(e.what()));
        return false;
    }
}

std::vector<model::Metric> MetricDAO::getByHostId(int host_id, int limit) {
    std::vector<model::Metric> metrics;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, host_id, timestamp, cpu_usage, mem_usage, disk_usage FROM metrics WHERE host_id = ? ORDER BY timestamp DESC LIMIT ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return metrics;
        }
        
        sqlite3_bind_int(stmt, 1, host_id);
        sqlite3_bind_int(stmt, 2, limit);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Metric metric;
            metric.id = sqlite3_column_int(stmt, 0);
            metric.host_id = sqlite3_column_int(stmt, 1);
            
            time_t timestamp = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            metric.timestamp = std::chrono::system_clock::from_time_t(timestamp);
            
            metric.cpu_usage = sqlite3_column_double(stmt, 3);
            metric.mem_usage = sqlite3_column_double(stmt, 4);
            metric.disk_usage = sqlite3_column_double(stmt, 5);
            
            metrics.push_back(metric);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("查询主机指标异常: " + std::string(e.what()));
    }
    return metrics;
}

std::vector<model::Metric> MetricDAO::getByHostIdAndTimeRange(int host_id, 
                                                           const std::chrono::system_clock::time_point& start_time,
                                                           const std::chrono::system_clock::time_point& end_time) {
    std::vector<model::Metric> metrics;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, host_id, timestamp, cpu_usage, mem_usage, disk_usage FROM metrics WHERE host_id = ? AND timestamp >= ? AND timestamp <= ? ORDER BY timestamp ASC";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return metrics;
        }
        
        sqlite3_bind_int(stmt, 1, host_id);
        
        auto start_str = std::to_string(std::chrono::system_clock::to_time_t(start_time));
        auto end_str = std::to_string(std::chrono::system_clock::to_time_t(end_time));
        
        sqlite3_bind_text(stmt, 2, start_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, end_str.c_str(), -1, SQLITE_TRANSIENT);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Metric metric;
            metric.id = sqlite3_column_int(stmt, 0);
            metric.host_id = sqlite3_column_int(stmt, 1);
            
            time_t timestamp = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            metric.timestamp = std::chrono::system_clock::from_time_t(timestamp);
            
            metric.cpu_usage = sqlite3_column_double(stmt, 3);
            metric.mem_usage = sqlite3_column_double(stmt, 4);
            metric.disk_usage = sqlite3_column_double(stmt, 5);
            
            metrics.push_back(metric);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("查询主机时间范围指标异常: " + std::string(e.what()));
    }
    return metrics;
}

std::vector<model::Metric> MetricDAO::getLatestMetrics(int limit) {
    std::vector<model::Metric> metrics;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, host_id, timestamp, cpu_usage, mem_usage, disk_usage FROM metrics ORDER BY timestamp DESC LIMIT ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return metrics;
        }
        
        sqlite3_bind_int(stmt, 1, limit);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Metric metric;
            metric.id = sqlite3_column_int(stmt, 0);
            metric.host_id = sqlite3_column_int(stmt, 1);
            
            time_t timestamp = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            metric.timestamp = std::chrono::system_clock::from_time_t(timestamp);
            
            metric.cpu_usage = sqlite3_column_double(stmt, 3);
            metric.mem_usage = sqlite3_column_double(stmt, 4);
            metric.disk_usage = sqlite3_column_double(stmt, 5);
            
            metrics.push_back(metric);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("查询最新指标异常: " + std::string(e.what()));
    }
    return metrics;
}

bool MetricDAO::deleteOldMetrics(const std::chrono::system_clock::time_point& before_time) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "DELETE FROM metrics WHERE timestamp < ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return false;
        }
        
        auto before_str = std::to_string(std::chrono::system_clock::to_time_t(before_time));
        sqlite3_bind_text(stmt, 1, before_str.c_str(), -1, SQLITE_TRANSIENT);
        
        result = sqlite3_step(stmt);
        int deleted_count = sqlite3_changes(db);
        
        if (result == SQLITE_DONE) {
            LOG_INFO("删除旧指标数据成功，条数: " + std::to_string(deleted_count));
        } else {
            LOG_ERROR("删除旧指标数据失败: " + std::string(sqlite3_errmsg(db)));
            sqlite3_finalize(stmt);
            return false;
        }
        
        sqlite3_finalize(stmt);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("删除旧指标数据异常: " + std::string(e.what()));
        return false;
    }
}

} // namespace dao
