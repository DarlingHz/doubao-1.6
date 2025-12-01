#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace model {

struct Host {
    int id = 0;
    std::string name;
    std::string ip;
    std::string environment;
    std::string tags; // 用逗号分隔的标签列表
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    bool is_deleted = false;
};

} // namespace model
