#ifndef SUBMISSION_REPOSITORY_H
#define SUBMISSION_REPOSITORY_H

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "submission.h"

// 用户统计信息结构体
struct UserStats {
    int total_submissions = 0;
    int ac_submissions = 0;
    int failed_submissions = 0;
    int partial_submissions = 0;

    struct DifficultyStats {
        int ac_count = 0;
        int total_count = 0;
        double pass_rate = 0.0;
    };

    DifficultyStats easy_stats;
    DifficultyStats medium_stats;
    DifficultyStats hard_stats;

    struct DailyStats {
        std::chrono::system_clock::time_point date;
        int submission_count = 0;
    };

    std::vector<DailyStats> recent_30_days_stats;
};

class SubmissionRepository {
public:
    SubmissionRepository() = default;
    virtual ~SubmissionRepository() = default;

    // 禁止拷贝和移动
    SubmissionRepository(const SubmissionRepository&) = delete;
    SubmissionRepository& operator=(const SubmissionRepository&) = delete;
    SubmissionRepository(SubmissionRepository&&) = delete;
    SubmissionRepository& operator=(SubmissionRepository&&) = delete;

    // 创建提交记录
    virtual bool CreateSubmission(Submission& submission) = 0;

    // 根据ID获取提交记录
    virtual std::shared_ptr<Submission> GetSubmissionById(int id) = 0;

    // 分页查询用户的提交记录
    virtual std::vector<std::shared_ptr<Submission>> GetUserSubmissions(
        int user_id,
        int page = 1,
        int page_size = 10,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point()) = 0;

    // 获取用户提交记录总数
    virtual int GetUserSubmissionCount(
        int user_id,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point()) = 0;

    // 获取用户的统计信息
    virtual UserStats GetUserStats(int user_id) = 0;
};

#endif // SUBMISSION_REPOSITORY_H