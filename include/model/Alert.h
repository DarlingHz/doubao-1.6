#pragma once
#include <string>
#include <chrono>
#include <optional>

namespace model {

enum class AlertStatus {
    TRIGGERED, // 已触发
    ACKED,     // 已确认
    RESOLVED   // 已解决
};

struct Alert {
    int id = 0;
    int rule_id = 0;
    int host_id = 0;
    AlertStatus status = AlertStatus::TRIGGERED;
    std::chrono::system_clock::time_point first_trigger_at;
    std::chrono::system_clock::time_point last_trigger_at;
    std::optional<std::chrono::system_clock::time_point> resolved_at;
    double last_metric_value = 0.0;
    std::string message;
};

} // namespace model
