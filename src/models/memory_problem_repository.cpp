#include "memory_problem_repository.h"
#include <algorithm>
#include <chrono>

bool MemoryProblemRepository::CreateProblem(Problem& problem) {
    std::lock_guard<std::mutex> lock(mutex_);

    problem.SetId(next_id_++);
    auto now = std::chrono::system_clock::now();
    problem.SetCreatedAt(now);
    problem.SetUpdatedAt(now);

    auto problem_ptr = std::make_shared<Problem>(problem);
    problems_[problem.GetId()] = problem_ptr;

    return true;
}

std::shared_ptr<Problem> MemoryProblemRepository::GetProblemById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = problems_.find(id);
    if (it == problems_.end() || it->second->IsDeleted()) {
        return nullptr;
    }

    return it->second;
}

bool MemoryProblemRepository::UpdateProblem(const Problem& problem) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = problems_.find(problem.GetId());
    if (it == problems_.end() || it->second->IsDeleted()) {
        return false;
    }

    *it->second = problem;
    it->second->SetUpdatedAt(std::chrono::system_clock::now());

    return true;
}

bool MemoryProblemRepository::DeleteProblem(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = problems_.find(id);
    if (it == problems_.end() || it->second->IsDeleted()) {
        return false;
    }

    it->second->SetIsDeleted(true);
    it->second->SetUpdatedAt(std::chrono::system_clock::now());

    return true;
}

std::vector<std::shared_ptr<Problem>> MemoryProblemRepository::GetProblems(
    int page, int page_size,
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto filtered = FilterProblems(keyword, difficulty, tags);

    // 分页
    int start_index = (page - 1) * page_size;
    int end_index = std::min(start_index + page_size, (int)filtered.size());

    if (start_index >= (int)filtered.size()) {
        return {};
    }

    return std::vector<std::shared_ptr<Problem>>(filtered.begin() + start_index, filtered.begin() + end_index);
}

int MemoryProblemRepository::GetProblemCount(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto filtered = FilterProblems(keyword, difficulty, tags);
    return filtered.size();
}

std::vector<std::shared_ptr<Problem>> MemoryProblemRepository::SearchProblems(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags,
    int page, int page_size) {
    // 搜索功能与GetProblems类似，只是默认必须提供关键字
    return GetProblems(page, page_size, keyword, difficulty, tags);
}

int MemoryProblemRepository::GetSearchResultCount(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) {
    // 搜索结果总数与GetProblemCount类似，只是默认必须提供关键字
    return GetProblemCount(keyword, difficulty, tags);
}

std::vector<std::shared_ptr<Problem>> MemoryProblemRepository::FilterProblems(
    const std::string& keyword,
    Difficulty difficulty,
    const std::vector<std::string>& tags) const {
    std::vector<std::shared_ptr<Problem>> filtered;

    for (const auto& pair : problems_) {
        auto problem = pair.second;

        if (problem->IsDeleted()) {
            continue;
        }

        // 难度过滤
        if (difficulty != Difficulty::EASY || !tags.empty() || !keyword.empty()) {
            if (problem->GetDifficulty() != difficulty) {
                continue;
            }
        }

        // 标签过滤
        if (!tags.empty()) {
            bool has_all_tags = true;
            for (const auto& tag : tags) {
                auto problem_tags = problem->GetTags();
                if (std::find(problem_tags.begin(), problem_tags.end(), tag) == problem_tags.end()) {
                    has_all_tags = false;
                    break;
                }
            }

            if (!has_all_tags) {
                continue;
            }
        }

        // 关键字过滤
        if (!keyword.empty()) {
            std::string title = problem->GetTitle();
            std::string description = problem->GetDescription();

            if (title.find(keyword) == std::string::npos &&
                description.find(keyword) == std::string::npos) {
                continue;
            }
        }

        filtered.push_back(problem);
    }

    // 按创建时间降序排序
    std::sort(filtered.begin(), filtered.end(), [](const std::shared_ptr<Problem>& a, const std::shared_ptr<Problem>& b) {
        return a->GetCreatedAt() > b->GetCreatedAt();
    });

    return filtered;
}