#include "memory_submission_repository.h"
#include <algorithm>
#include <chrono>

bool MemorySubmissionRepository::CreateSubmission(Submission& submission) {
    std::lock_guard<std::mutex> lock(mutex_);

    submission.SetId(next_id_++);
    auto now = std::chrono::system_clock::now();
    submission.SetCreatedAt(now);

    auto submission_ptr = std::make_shared<Submission>(submission);
    submissions_[submission.GetId()] = submission_ptr;

    return true;
}

std::shared_ptr<Submission> MemorySubmissionRepository::GetSubmissionById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = submissions_.find(id);
    if (it == submissions_.end()) {
        return nullptr;
    }

    return it->second;
}

std::vector<std::shared_ptr<Submission>> MemorySubmissionRepository::GetUserSubmissions(
    int user_id,
    int page,
    int page_size,
    int problem_id,
    SubmissionStatus status,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto filtered = FilterUserSubmissions(user_id, problem_id, status, start_time, end_time);

    // 分页
    int start_index = (page - 1) * page_size;
    int end_index = std::min(start_index + page_size, (int)filtered.size());

    if (start_index >= (int)filtered.size()) {
        return {};
    }

    return std::vector<std::shared_ptr<Submission>>(filtered.begin() + start_index, filtered.begin() + end_index);
}

int MemorySubmissionRepository::GetUserSubmissionCount(
    int user_id,
    int problem_id,
    SubmissionStatus status,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto filtered = FilterUserSubmissions(user_id, problem_id, status, start_time, end_time);
    return filtered.size();
}

UserStats MemorySubmissionRepository::GetUserStats(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    UserStats stats;
    std::unordered_map<std::string, int> daily_counts;

    // 获取用户所有提交记录
    auto user_submissions = FilterUserSubmissions(user_id, 0, SubmissionStatus::PENDING, std::chrono::system_clock::time_point(), std::chrono::system_clock::time_point());

    for (const auto& submission : user_submissions) {
        stats.total_submissions++;

        switch (submission->GetStatus()) {
            case SubmissionStatus::AC:
                stats.ac_submissions++;
                break;
            case SubmissionStatus::PARTIAL:
                stats.partial_submissions++;
                break;
            case SubmissionStatus::FAILED:
                stats.failed_submissions++;
                break;
            default:
                break;
        }

        // 按日期统计
        auto time_t = std::chrono::system_clock::to_time_t(submission->GetCreatedAt());
        std::tm tm = *std::localtime(&time_t);
        char date_str[11];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm);
        daily_counts[date_str]++;
    }

    // 最近30天的统计
    auto now = std::chrono::system_clock::now();
    for (int i = 29; i >= 0; i--) {
        auto date = now - std::chrono::hours(24 * i);
        auto time_t = std::chrono::system_clock::to_time_t(date);
        std::tm tm = *std::localtime(&time_t);
        char date_str[11];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm);

        UserStats::DailyStats daily_stats;
        daily_stats.date = date;
        daily_stats.submission_count = daily_counts[date_str];
        stats.recent_30_days_stats.push_back(daily_stats);
    }

    return stats;
}

std::vector<std::shared_ptr<Submission>> MemorySubmissionRepository::FilterUserSubmissions(
    int user_id,
    int problem_id,
    SubmissionStatus status,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) const {
    std::vector<std::shared_ptr<Submission>> filtered;

    for (const auto& pair : submissions_) {
        auto submission = pair.second;

        // 用户ID过滤
        if (submission->GetUserId() != user_id) {
            continue;
        }

        // 题目ID过滤
        if (problem_id > 0 && submission->GetProblemId() != problem_id) {
            continue;
        }

        // 状态过滤
        if (status != SubmissionStatus::PENDING && submission->GetStatus() != status) {
            continue;
        }

        // 开始时间过滤
        if (start_time != std::chrono::system_clock::time_point() && submission->GetCreatedAt() < start_time) {
            continue;
        }

        // 结束时间过滤
        if (end_time != std::chrono::system_clock::time_point() && submission->GetCreatedAt() > end_time) {
            continue;
        }

        filtered.push_back(submission);
    }

    // 按创建时间降序排序
    std::sort(filtered.begin(), filtered.end(), [](const std::shared_ptr<Submission>& a, const std::shared_ptr<Submission>& b) {
        return a->GetCreatedAt() > b->GetCreatedAt();
    });

    return filtered;
}