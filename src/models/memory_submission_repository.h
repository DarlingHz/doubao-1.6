#ifndef MEMORY_SUBMISSION_REPOSITORY_H
#define MEMORY_SUBMISSION_REPOSITORY_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "submission_repository.h"

class MemorySubmissionRepository : public SubmissionRepository {
public:
    MemorySubmissionRepository() = default;
    ~MemorySubmissionRepository() override = default;

    // 创建提交记录
    bool CreateSubmission(Submission& submission) override;

    // 根据ID获取提交记录
    std::shared_ptr<Submission> GetSubmissionById(int id) override;

    // 分页查询用户的提交记录
    std::vector<std::shared_ptr<Submission>> GetUserSubmissions(
        int user_id,
        int page = 1,
        int page_size = 10,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point()) override;

    // 获取用户提交记录总数
    int GetUserSubmissionCount(
        int user_id,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point()) override;

    // 获取用户的统计信息
    UserStats GetUserStats(int user_id) override;

private:
    // 过滤用户提交记录
    std::vector<std::shared_ptr<Submission>> FilterUserSubmissions(
        int user_id,
        int problem_id,
        SubmissionStatus status,
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time) const;

private:
    std::unordered_map<int, std::shared_ptr<Submission>> submissions_;
    std::mutex mutex_;
    int next_id_ = 1;
};

#endif // MEMORY_SUBMISSION_REPOSITORY_H