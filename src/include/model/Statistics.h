#ifndef STATISTICS_H
#define STATISTICS_H

#include <string>
#include <vector>
#include <map>

namespace model {

struct DailyCardCount {
    std::string date; // 日期，格式：YYYY-MM-DD
    int count;        // 该日期新增卡片数
};

struct TagStatistics {
    std::string tagName; // 标签名称
    int count;           // 卡片数量
};

class Statistics {
public:
    Statistics() = default;
    
    int getTotalCards() const { return totalCards_; }
    void setTotalCards(int totalCards) { totalCards_ = totalCards; }
    
    const std::vector<TagStatistics>& getTopTags() const { return topTags_; }
    void setTopTags(const std::vector<TagStatistics>& topTags) { topTags_ = topTags; }
    
    const std::vector<DailyCardCount>& getRecentDailyCounts() const { return recentDailyCounts_; }
    void setRecentDailyCounts(const std::vector<DailyCardCount>& dailyCounts) { recentDailyCounts_ = dailyCounts; }
    
private:
    int totalCards_ = 0;                           // 总卡片数
    std::vector<TagStatistics> topTags_;           // 热门标签统计
    std::vector<DailyCardCount> recentDailyCounts_; // 最近N天新增卡片数
};

} // namespace model

#endif // STATISTICS_H