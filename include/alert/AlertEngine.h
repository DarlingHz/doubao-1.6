#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <dao/MetricDAO.h>
#include <dao/AlertRuleDAO.h>
#include <dao/AlertDAO.h>
#include <dao/HostDAO.h>
#include <config/Config.h>

namespace alert {

class AlertEngine {
public:
    AlertEngine(std::shared_ptr<dao::MetricDAO> metric_dao,
                std::shared_ptr<dao::AlertRuleDAO> rule_dao,
                std::shared_ptr<dao::AlertDAO> alert_dao,
                std::shared_ptr<dao::HostDAO> host_dao,
                const config::Config& config);
    
    ~AlertEngine();
    
    void start();
    void stop();
    
private:
    void run();
    bool evaluateRule(const model::AlertRule& rule, const std::vector<model::Metric>& metrics);
    void processAlert(const model::AlertRule& rule, const model::Metric& metric);
    
    std::shared_ptr<dao::MetricDAO> metric_dao_;
    std::shared_ptr<dao::AlertRuleDAO> rule_dao_;
    std::shared_ptr<dao::AlertDAO> alert_dao_;
    std::shared_ptr<dao::HostDAO> host_dao_;
    config::Config config_;
    
    std::thread worker_thread_;
    std::atomic<bool> running_ = false;
    std::chrono::system_clock::time_point last_check_time_;
};

} // namespace alert
