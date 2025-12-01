#pragma once
#include <model/AlertRule.h>
#include <dao/DbConnectionPool.h>
#include <vector>
#include <optional>

namespace dao {

class AlertRuleDAO {
public:
    explicit AlertRuleDAO(std::shared_ptr<DbConnectionPool> pool);
    
    bool createTable();
    int create(const model::AlertRule& rule);
    std::optional<model::AlertRule> getById(int id);
    std::vector<model::AlertRule> getAll(bool only_enabled = false);
    std::vector<model::AlertRule> getByHostId(int host_id, bool only_enabled = false);
    bool update(const model::AlertRule& rule);
    bool deleteById(int id);
    
private:
    std::shared_ptr<DbConnectionPool> pool_;
};

} // namespace dao
