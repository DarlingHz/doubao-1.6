#include "submission_service.h"
#include <chrono>

bool SubmissionService::CreateSubmission(int user_id, int problem_id, SubmissionStatus status, int time_spent_seconds, const std::string& note, Submission& submission) {
    // 参数验证
    if (user_id <= 0 || problem_id <= 0 || time_spent_seconds < 0) {
        return false;
    }

    // 检查题目是否存在
    auto problem = problem_repository_->GetProblemById(problem_id);
    if (!problem || problem->IsDeleted()) {
        return false;
    }

    // 创建提交记录对象
    submission.SetUserId(user_id);
    submission.SetProblemId(problem_id);
    submission.SetStatus(status);
    submission.SetTimeSpentSeconds(time_spent_seconds);
    submission.SetNote(note);

    // 保存到数据库
    return submission_repository_->CreateSubmission(submission);
}

std::shared_ptr<Submission> SubmissionService::GetSubmissionById(int id) {
    // 参数验证
    if (id <= 0) {
        return nullptr;
    }

    // 从数据库获取提交记录
    return submission_repository_->GetSubmissionById(id);
}

std::vector<std::shared_ptr<Submission>> SubmissionService::GetUserSubmissions(
    int user_id,
    int page,
    int page_size,
    int problem_id,
    SubmissionStatus status,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {
    // 参数验证
    if (user_id <= 0 || page <= 0 || page_size <= 0) {
        return {};
    }

    // 从数据库获取用户提交记录列表
    return submission_repository_->GetUserSubmissions(user_id, page, page_size, problem_id, status, start_time, end_time);
}

int SubmissionService::GetUserSubmissionCount(
    int user_id,
    int problem_id,
    SubmissionStatus status,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {
    // 参数验证
    if (user_id <= 0) {
        return 0;
    }

    // 从数据库获取用户提交记录总数
    return submission_repository_->GetUserSubmissionCount(user_id, problem_id, status, start_time, end_time);
}

UserStats SubmissionService::GetUserStats(int user_id) {
    // 参数验证
    if (user_id <= 0) {
        return UserStats();
    }

    // 从数据库获取用户统计信息
    return submission_repository_->GetUserStats(user_id);
}