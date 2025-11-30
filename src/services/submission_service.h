#ifndef SUBMISSION_SERVICE_H
#define SUBMISSION_SERVICE_H

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "models/submission.h"
#include "models/submission_repository.h"
#include "models/problem_repository.h"

class SubmissionService {
public:
    SubmissionService(
        std::shared_ptr<SubmissionRepository> submission_repository,
        std::shared_ptr<ProblemRepository> problem_repository)
        : submission_repository_(submission_repository)
        , problem_repository_(problem_repository) {
    }

    ~SubmissionService() = default;

    // 禁止拷贝和移动
    SubmissionService(const SubmissionService&) = delete;
    SubmissionService& operator=(const SubmissionService&) = delete;
    SubmissionService(SubmissionService&&) = delete;
    SubmissionService& operator=(SubmissionService&&) = delete;

    // 创建提交记录
    bool CreateSubmission(int user_id, int problem_id, SubmissionStatus status, int time_spent_seconds, const std::string& note, Submission& submission);

    // 根据ID获取提交记录
    std::shared_ptr<Submission> GetSubmissionById(int id);

    // 分页查询用户的提交记录
    std::vector<std::shared_ptr<Submission>> GetUserSubmissions(
        int user_id,
        int page = 1,
        int page_size = 10,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point());

    // 获取用户提交记录总数
    int GetUserSubmissionCount(
        int user_id,
        int problem_id = 0,
        SubmissionStatus status = SubmissionStatus::PENDING,
        const std::chrono::system_clock::time_point& start_time = std::chrono::system_clock::time_point(),
        const std::chrono::system_clock::time_point& end_time = std::chrono::system_clock::time_point());

    // 获取用户的统计信息
    UserStats GetUserStats(int user_id);

private:
    std::shared_ptr<SubmissionRepository> submission_repository_;
    std::shared_ptr<ProblemRepository> problem_repository_;
};

#endif // SUBMISSION_SERVICE_H