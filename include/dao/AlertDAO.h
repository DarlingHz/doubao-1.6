#pragma once
#include <model/Alert.h>
#include <dao/DbConnectionPool.h>
#include <vector>
#include <optional>
#include <chrono>

namespace dao {

class AlertDAO {
public:
    explicit AlertDAO(std::shared_ptr<DbConnectionPool> pool);
    
    bool createTable();
    int create(const model::Alert& alert);
    std::optional<model::Alert> getById(int id);
    std::vector<model::Alert> getAll(model::AlertStatus status = model::AlertStatus::TRIGGERED, int limit = 100, int offset = 0);
    std::vector<model::Alert> getByHostId(int host_id, model::AlertStatus status = model::AlertStatus::TRIGGERED, int limit = 100, int offset = 0);
    std::vector<model::Alert> getByTimeRange(const std::chrono::system_clock::time_point& start_time,
                                           const std::chrono::system_clock::time_point& end_time,
                                           model::AlertStatus status = model::AlertStatus::TRIGGERED,
                                           int limit = 100, int offset = 0);
    bool updateStatus(int id, model::AlertStatus status);
    bool updateLastTrigger(int id, const std::chrono::system_clock::time_point& timestamp, double metric_value);
    
private:
    std::shared_ptr<DbConnectionPool> pool_;
};

} // namespace dao
