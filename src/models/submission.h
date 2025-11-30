#ifndef SUBMISSION_H
#define SUBMISSION_H

#include <string>
#include <chrono>

enum class SubmissionStatus {
    AC,         // 完全正确
    PARTIAL,    // 部分正确
    FAILED,     // 失败
    PENDING     // 待处理
};

class Submission {
public:
    Submission() = default;
    ~Submission() = default;

    // 获取提交ID
    int GetId() const;

    // 设置提交ID
    void SetId(int id);

    // 获取用户ID
    int GetUserId() const;

    // 设置用户ID
    void SetUserId(int user_id);

    // 获取题目ID
    int GetProblemId() const;

    // 设置题目ID
    void SetProblemId(int problem_id);

    // 获取提交状态
    SubmissionStatus GetStatus() const;

    // 设置提交状态
    void SetStatus(SubmissionStatus status);

    // 获取做题所用时间（秒）
    int GetTimeSpentSeconds() const;

    // 设置做题所用时间（秒）
    void SetTimeSpentSeconds(int time_spent_seconds);

    // 获取备注
    std::string GetNote() const;

    // 设置备注
    void SetNote(const std::string& note);

    // 获取提交时间
    std::chrono::system_clock::time_point GetCreatedAt() const;

    // 设置提交时间
    void SetCreatedAt(const std::chrono::system_clock::time_point& created_at);

private:
    int id_ = 0;
    int user_id_ = 0;
    int problem_id_ = 0;
    SubmissionStatus status_ = SubmissionStatus::PENDING;
    int time_spent_seconds_ = 0;
    std::string note_;
    std::chrono::system_clock::time_point created_at_;
};

#endif // SUBMISSION_H
