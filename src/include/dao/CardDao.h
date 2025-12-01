#ifndef CARD_DAO_H
#define CARD_DAO_H

#include <memory>
#include <vector>
#include <string>
#include "model/Card.h"
#include "model/RequestResponse.h"

namespace dao {

class CardDao {
public:
    virtual ~CardDao() = default;
    
    // 根据ID查找卡片
    virtual std::shared_ptr<model::Card> findById(int id) = 0;
    
    // 根据用户ID和卡片ID查找卡片
    virtual std::shared_ptr<model::Card> findByUserIdAndCardId(int userId, int cardId) = 0;
    
    // 创建新卡片
    virtual int create(const model::Card& card) = 0;
    
    // 更新卡片
    virtual bool update(const model::Card& card) = 0;
    
    // 删除卡片
    virtual bool deleteById(int id) = 0;
    
    // 分页查询卡片列表
    virtual model::PaginatedResponse<model::Card> findByUserId(int userId, const model::CardListQuery& query) = 0;
    
    // 根据标签查询卡片
    virtual std::vector<std::shared_ptr<model::Card>> findByTags(int userId, const std::vector<std::string>& tags) = 0;
    
    // 搜索卡片
    virtual model::PaginatedResponse<model::Card> search(int userId, const std::string& keyword, const model::CardListQuery& query) = 0;
    
    // 获取用户卡片总数
    virtual int countByUserId(int userId) = 0;
    
    // 获取最近N天的新增卡片数
    virtual std::vector<std::pair<std::string, int>> getRecentDailyCount(int userId, int days) = 0;
};

} // namespace dao

#endif // CARD_DAO_H