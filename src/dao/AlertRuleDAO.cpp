#include <dao/AlertRuleDAO.h>
#include <util/Logger.h>

namespace dao {

AlertRuleDAO::AlertRuleDAO(std::shared_ptr<DbConnectionPool> pool)
    : pool_(pool) {
}

bool AlertRuleDAO::createTable() {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS alert_rules (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                target_host_id INTEGER DEFAULT 0,
                metric_type INTEGER NOT NULL,
                threshold REAL NOT NULL,
                comparison INTEGER NOT NULL,
                duration_sec INTEGER NOT NULL,
                enabled INTEGER DEFAULT 1,
                created_at TEXT NOT NULL
            );
            
            CREATE INDEX IF NOT EXISTS idx_alert_rules_enabled ON alert_rules(enabled);
            CREATE INDEX IF NOT EXISTS idx_alert_rules_host_id ON alert_rules(target_host_id, enabled);
        )";
        
        char* errmsg = nullptr;
        int result = sqlite3_exec(db, sql, nullptr, nullptr, &errmsg);
        if (result != SQLITE_OK) {
            LOG_ERROR("创建alert_rules表失败: " + std::string(errmsg));
            sqlite3_free(errmsg);
            return false;
        }
        
        LOG_INFO("alert_rules表创建成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("创建alert_rules表异常: " + std::string(e.what()));
        return false;
    }
}

int AlertRuleDAO::create(const model::AlertRule& rule) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        auto now = std::chrono::system_clock::now();
        auto now_str = std::to_string(std::chrono::system_clock::to_time_t(now));
        
        const char* sql = "INSERT INTO alert_rules (name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return 0;
        }
        
        sqlite3_bind_text(stmt, 1, rule.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, rule.target_host_id);
        sqlite3_bind_int(stmt, 3, static_cast<int>(rule.metric_type));
        sqlite3_bind_double(stmt, 4, rule.threshold);
        sqlite3_bind_int(stmt, 5, static_cast<int>(rule.comparison));
        sqlite3_bind_int(stmt, 6, rule.duration_sec);
        sqlite3_bind_int(stmt, 7, rule.enabled ? 1 : 0);
        sqlite3_bind_text(stmt, 8, now_str.c_str(), -1, SQLITE_TRANSIENT);
        
        result = sqlite3_step(stmt);
        int id = 0;
        if (result == SQLITE_DONE) {
            id = sqlite3_last_insert_rowid(db);
            LOG_INFO("创建告警规则成功，ID: " + std::to_string(id));
        } else {
            LOG_ERROR("执行SQL失败: " + std::string(sqlite3_errmsg(db)));
        }
        
        sqlite3_finalize(stmt);
        return id;
    } catch (const std::exception& e) {
        LOG_ERROR("创建告警规则异常: " + std::string(e.what()));
        return 0;
    }
}

std::optional<model::AlertRule> AlertRuleDAO::getById(int id) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "SELECT id, name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at FROM alert_rules WHERE id = ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return std::nullopt;
        }
        
        sqlite3_bind_int(stmt, 1, id);
        
        model::AlertRule rule;
        bool found = false;
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            rule.id = sqlite3_column_int(stmt, 0);
            rule.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rule.target_host_id = sqlite3_column_int(stmt, 2);
            rule.metric_type = static_cast<model::MetricType>(sqlite3_column_int(stmt, 3));
            rule.threshold = sqlite3_column_double(stmt, 4);
            rule.comparison = static_cast<model::Comparison>(sqlite3_column_int(stmt, 5));
            rule.duration_sec = sqlite3_column_int(stmt, 6);
            rule.enabled = sqlite3_column_int(stmt, 7) != 0;
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
            rule.created_at = std::chrono::system_clock::from_time_t(created_at);
            
            found = true;
        }
        
        sqlite3_finalize(stmt);
        
        if (found) {
            return rule;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("查询告警规则异常: " + std::string(e.what()));
        return std::nullopt;
    }
}

std::vector<model::AlertRule> AlertRuleDAO::getAll(bool only_enabled) {
    std::vector<model::AlertRule> rules;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = only_enabled 
            ? "SELECT id, name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at FROM alert_rules WHERE enabled = 1" 
            : "SELECT id, name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at FROM alert_rules";
        
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return rules;
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::AlertRule rule;
            rule.id = sqlite3_column_int(stmt, 0);
            rule.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rule.target_host_id = sqlite3_column_int(stmt, 2);
            rule.metric_type = static_cast<model::MetricType>(sqlite3_column_int(stmt, 3));
            rule.threshold = sqlite3_column_double(stmt, 4);
            rule.comparison = static_cast<model::Comparison>(sqlite3_column_int(stmt, 5));
            rule.duration_sec = sqlite3_column_int(stmt, 6);
            rule.enabled = sqlite3_column_int(stmt, 7) != 0;
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
            rule.created_at = std::chrono::system_clock::from_time_t(created_at);
            
            rules.push_back(rule);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("查询所有告警规则异常: " + std::string(e.what()));
    }
    return rules;
}

std::vector<model::AlertRule> AlertRuleDAO::getByHostId(int host_id, bool only_enabled) {
    std::vector<model::AlertRule> rules;
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = only_enabled 
            ? "SELECT id, name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at FROM alert_rules WHERE (target_host_id = 0 OR target_host_id = ?) AND enabled = 1" 
            : "SELECT id, name, target_host_id, metric_type, threshold, comparison, duration_sec, enabled, created_at FROM alert_rules WHERE target_host_id = 0 OR target_host_id = ?";
        
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return rules;
        }
        
        sqlite3_bind_int(stmt, 1, host_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::AlertRule rule;
            // 与getAll相同的解析逻辑
            rule.id = sqlite3_column_int(stmt, 0);
            rule.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rule.target_host_id = sqlite3_column_int(stmt, 2);
            rule.metric_type = static_cast<model::MetricType>(sqlite3_column_int(stmt, 3));
            rule.threshold = sqlite3_column_double(stmt, 4);
            rule.comparison = static_cast<model::Comparison>(sqlite3_column_int(stmt, 5));
            rule.duration_sec = sqlite3_column_int(stmt, 6);
            rule.enabled = sqlite3_column_int(stmt, 7) != 0;
            
            time_t created_at = std::stoll(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
            rule.created_at = std::chrono::system_clock::from_time_t(created_at);
            
            rules.push_back(rule);
        }
        
        sqlite3_finalize(stmt);
    } catch (const std::exception& e) {
        LOG_ERROR("按主机查询告警规则异常: " + std::string(e.what()));
    }
    return rules;
}

bool AlertRuleDAO::update(const model::AlertRule& rule) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "UPDATE alert_rules SET name = ?, target_host_id = ?, metric_type = ?, threshold = ?, comparison = ?, duration_sec = ?, enabled = ? WHERE id = ?";
        sqlite3_stmt* stmt = nullptr;
        
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            LOG_ERROR("准备SQL语句失败: " + std::string(sqlite3_errmsg(db)));
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, rule.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, rule.target_host_id);
        sqlite3_bind_int(stmt, 3, static_cast<int>(rule.metric_type));
        sqlite3_bind_double(stmt, 4, rule.threshold);
        sqlite3_bind_int(stmt, 5, static_cast<int>(rule.comparison));
        sqlite3_bind_int(stmt, 6, rule.duration_sec);
        sqlite3_bind_int(stmt, 7, rule.enabled ? 1 : 0);
        sqlite3_bind_int(stmt, 8, rule.id);
        
        result = sqlite3_step(stmt);
        bool success = (result == SQLITE_DONE && sqlite3_changes(db) > 0);
        
        if (success) {
            LOG_INFO("更新告警规则成功，ID: " + std::to_string(rule.id));
        } else {
            LOG_ERROR("更新告警规则失败，ID: " + std::to_string(rule.id));
        }
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("更新告警规则异常: " + std::string(e.what()));
        return false;
    }
}

bool AlertRuleDAO::deleteById(int id) {
    try {
        auto connection = pool_->getConnection();
        sqlite3* db = connection->get();
        
        const char* sql = "DELETE FROM alert_rules WHERE id = ?";
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
            LOG_INFO("删除告警规则成功，ID: " + std::to_string(id));
        } else {
            LOG_ERROR("删除告警规则失败，ID: " + std::to_string(id));
        }
        
        sqlite3_finalize(stmt);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("删除告警规则异常: " + std::string(e.what()));
        return false;
    }
}

} // namespace dao
