#ifndef PROBLEM_H
#define PROBLEM_H

#include <string>
#include <vector>
#include <chrono>

enum class Difficulty {
    EASY,
    MEDIUM,
    HARD
};

class Problem {
public:
    Problem() = default;
    ~Problem() = default;

    // 获取题目ID
    int GetId() const;

    // 设置题目ID
    void SetId(int id);

    // 获取题目标题
    std::string GetTitle() const;

    // 设置题目标题
    void SetTitle(const std::string& title);

    // 获取题目描述
    std::string GetDescription() const;

    // 设置题目描述
    void SetDescription(const std::string& description);

    // 获取题目难度
    Difficulty GetDifficulty() const;

    // 设置题目难度
    void SetDifficulty(Difficulty difficulty);

    // 获取题目标签
    std::vector<std::string> GetTags() const;

    // 设置题目标签
    void SetTags(const std::vector<std::string>& tags);

    // 获取题目创建时间
    std::chrono::system_clock::time_point GetCreatedAt() const;

    // 设置题目创建时间
    void SetCreatedAt(const std::chrono::system_clock::time_point& created_at);

    // 获取题目更新时间
    std::chrono::system_clock::time_point GetUpdatedAt() const;

    // 设置题目更新时间
    void SetUpdatedAt(const std::chrono::system_clock::time_point& updated_at);

    // 获取题目是否被删除
    bool IsDeleted() const;

    // 设置题目是否被删除
    void SetIsDeleted(bool is_deleted);

private:
    int id_ = 0;
    std::string title_;
    std::string description_;
    Difficulty difficulty_ = Difficulty::EASY;
    std::vector<std::string> tags_;
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point updated_at_;
    bool is_deleted_ = false;
};

#endif // PROBLEM_H
