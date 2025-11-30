#ifndef PROBLEM_REPOSITORY_H
#define PROBLEM_REPOSITORY_H

#include <memory>
#include <vector>
#include <string>
#include "problem.h"

class ProblemRepository {
public:
    ProblemRepository() = default;
    virtual ~ProblemRepository() = default;

    // 禁止拷贝和移动
    ProblemRepository(const ProblemRepository&) = delete;
    ProblemRepository& operator=(const ProblemRepository&) = delete;
    ProblemRepository(ProblemRepository&&) = delete;
    ProblemRepository& operator=(ProblemRepository&&) = delete;

    // 创建题目
    virtual bool CreateProblem(Problem& problem) = 0;

    // 根据ID获取题目
    virtual std::shared_ptr<Problem> GetProblemById(int id) = 0;

    // 更新题目信息
    virtual bool UpdateProblem(const Problem& problem) = 0;

    // 删除题目（软删除）
    virtual bool DeleteProblem(int id) = 0;

    // 分页查询题目列表
    virtual std::vector<std::shared_ptr<Problem>> GetProblems(
        int page = 1, int page_size = 10,
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) = 0;

    // 获取题目总数
    virtual int GetProblemCount(
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) = 0;

    // 搜索题目
    virtual std::vector<std::shared_ptr<Problem>> SearchProblems(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {},
        int page = 1, int page_size = 10) = 0;

    // 获取搜索结果总数
    virtual int GetSearchResultCount(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {}) = 0;
};

#endif // PROBLEM_REPOSITORY_H