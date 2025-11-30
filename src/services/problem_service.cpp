#include "problem_service.h"
#include <chrono>

bool ProblemService::CreateProblem(const std::string& title, const std::string& description, Difficulty difficulty, const std::vector<std::string>& tags, Problem& problem) {
    // 参数验证
    if (title.empty() || description.empty()) {
        return false;
    }

    // 创建题目对象
    problem.SetTitle(title);
    problem.SetDescription(description);
    problem.SetDifficulty(difficulty);
    problem.SetTags(tags);

    // 保存到数据库
    return problem_repository_->CreateProblem(problem);
}

std::shared_ptr<Problem> ProblemService::GetProblemById(int id) {
    // 参数验证
    if (id <= 0) {
        return nullptr;
    }

    // 从数据库获取题目
    return problem_repository_->GetProblemById(id);
}

bool ProblemService::UpdateProblem(int id, const std::string& title, const std::string& description, Difficulty difficulty, const std::vector<std::string>& tags) {
    // 参数验证
    if (id <= 0 || title.empty() || description.empty()) {
        return false;
    }

    // 从数据库获取题目
    auto problem = problem_repository_->GetProblemById(id);
    if (!problem) {
        return false;
    }

    // 更新题目信息
    problem->SetTitle(title);
    problem->SetDescription(description);
    problem->SetDifficulty(difficulty);
    problem->SetTags(tags);

    // 保存到数据库
    return problem_repository_->UpdateProblem(*problem);
}

bool ProblemService::DeleteProblem(int id) {
    // 参数验证
    if (id <= 0) {
        return false;
    }

    // 从数据库删除题目（软删除）
    return problem_repository_->DeleteProblem(id);
}

std::vector<std::shared_ptr<Problem>> ProblemService::GetProblems(
    int page,
    int page_size,
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    // 参数验证
    if (page <= 0 || page_size <= 0) {
        return {};
    }

    // 从数据库获取题目列表
    return problem_repository_->GetProblems(page, page_size, keyword, difficulty, tags);
}

int ProblemService::GetProblemCount(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    // 从数据库获取题目总数
    return problem_repository_->GetProblemCount(keyword, difficulty, tags);
}

std::vector<std::shared_ptr<Problem>> ProblemService::SearchProblems(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags,
    int page,
    int page_size) {
    // 参数验证
    if (keyword.empty() || page <= 0 || page_size <= 0) {
        return {};
    }

    // 从数据库搜索题目
    return problem_repository_->SearchProblems(keyword, difficulty, tags, page, page_size);
}

int ProblemService::GetSearchResultCount(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    // 参数验证
    if (keyword.empty()) {
        return 0;
    }

    // 从数据库获取搜索结果总数
    return problem_repository_->GetSearchResultCount(keyword, difficulty, tags);
}