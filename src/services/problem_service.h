#ifndef PROBLEM_SERVICE_H
#define PROBLEM_SERVICE_H

#include <memory>
#include <vector>
#include <string>
#include "models/problem.h"
#include "models/problem_repository.h"

class ProblemService {
public:
    ProblemService(std::shared_ptr<ProblemRepository> problem_repository)
        : problem_repository_(problem_repository) {
    }

    ~ProblemService() = default;

    // 禁止拷贝和移动
    ProblemService(const ProblemService&) = delete;
    ProblemService& operator=(const ProblemService&) = delete;
    ProblemService(ProblemService&&) = delete;
    ProblemService& operator=(ProblemService&&) = delete;

    // 创建题目
    bool CreateProblem(const std::string& title, const std::string& description, Difficulty difficulty, const std::vector<std::string>& tags, Problem& problem);

    // 根据ID获取题目
    std::shared_ptr<Problem> GetProblemById(int id);

    // 更新题目信息
    bool UpdateProblem(int id, const std::string& title, const std::string& description, Difficulty difficulty, const std::vector<std::string>& tags);

    // 删除题目（软删除）
    bool DeleteProblem(int id);

    // 分页查询题目列表
    std::vector<std::shared_ptr<Problem>> GetProblems(
        int page = 1,
        int page_size = 10,
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {});

    // 获取题目总数
    int GetProblemCount(
        const std::string& keyword = "",
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {});

    // 搜索题目
    std::vector<std::shared_ptr<Problem>> SearchProblems(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {},
        int page = 1,
        int page_size = 10);

    // 获取搜索结果总数
    int GetSearchResultCount(
        const std::string& keyword,
        Difficulty difficulty = Difficulty::EASY,
        const std::vector<std::string>& tags = {});

private:
    std::shared_ptr<ProblemRepository> problem_repository_;
};

#endif // PROBLEM_SERVICE_H