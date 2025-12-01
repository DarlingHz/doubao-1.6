#pragma once
#include <chrono>

namespace model {

struct Metric {
    int id = 0;
    int host_id = 0;
    std::chrono::system_clock::time_point timestamp;
    double cpu_usage = 0.0; // 0-100
    double mem_usage = 0.0; // 0-100
    double disk_usage = 0.0; // 0-100
};

} // namespace model
