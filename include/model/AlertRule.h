#pragma once
#include <string>
#include <chrono>

namespace model {

enum class MetricType {
    CPU,
    MEM,
    DISK
};

enum class Comparison {
    GREATER_THAN,          // >
    GREATER_THAN_OR_EQUAL, // >=
    LESS_THAN,             // <
    LESS_THAN_OR_EQUAL     // <=
};

struct AlertRule {
    int id = 0;
    std::string name;
    int target_host_id = 0; // 0表示适用于所有主机
    MetricType metric_type = MetricType::CPU;
    double threshold = 0.0;
    Comparison comparison = Comparison::GREATER_THAN;
    int duration_sec = 300; // 持续时间阈值，秒
    bool enabled = true;
    std::chrono::system_clock::time_point created_at;
};

} // namespace model
