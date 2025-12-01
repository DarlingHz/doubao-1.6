#pragma once
#include <model/Metric.h>
#include <dao/DbConnectionPool.h>
#include <vector>
#include <chrono>

namespace dao {

class MetricDAO {
public:
    explicit MetricDAO(std::shared_ptr<DbConnectionPool> pool);
    
    bool createTable();
    bool batchInsert(const std::vector<model::Metric>& metrics);
    std::vector<model::Metric> getByHostId(int host_id, int limit = 100);
    std::vector<model::Metric> getByHostIdAndTimeRange(int host_id, 
                                                    const std::chrono::system_clock::time_point& start_time,
                                                    const std::chrono::system_clock::time_point& end_time);
    std::vector<model::Metric> getLatestMetrics(int limit = 100);
    bool deleteOldMetrics(const std::chrono::system_clock::time_point& before_time);
    
private:
    std::shared_ptr<DbConnectionPool> pool_;
};

} // namespace dao
