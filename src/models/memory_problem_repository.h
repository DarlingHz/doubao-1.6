#ifndef MEMORY_PROBLEM_REPOSITORY_H
#define MEMORY_PROBLEM_REPOSITORY_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include "problem_repository.h"

class MemoryProblemRepository : public ProblemRepository {
public:
    MemoryProblemRepository() = default;
    ~MemoryProblemRepository() override = default;

    // 创建题目
    bool CreateProblem(Problem& problem) override;

    // 根据ID获取题目
    std::shared_ptr<Problem> GetProblemById(int id) override;

    // 更新题目信息
    bool UpdateProblem(const Problem& problem) override;

    // 删除题目（软删除）
    bool DeleteProblem(int id) override;

    // 分页查询题目列表
    std::vector<std::shared_ptr<Problem>> GetProblems(
        int page = 1, int page_size = 10,
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) override;

    // 获取题目总数
    int GetProblemCount(
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) override;

    // 搜索题目
    std::vector<std::shared_ptr<Problem>> SearchProblems(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {},
        int page = 1, int page_size = 10) override;

    // 获取搜索结果总数
    int GetSearchResultCount(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) override;

private:
    // 过滤题目
    std::vector<std::shared_ptr<Problem>> FilterProblems(
        const std::string& keyword,
        Difficulty difficulty,
        const std::vector<std::string>& tags) const;

private:
    std::unordered_map<int, std::shared_ptr<Problem>> problems_;
    std::mutex mutex_;
    int next_id_ = 1;
};

#endif // MEMORY_PROBLEM_REPOSITORY_H