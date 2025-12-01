#ifndef CARD_SERVICE_H
#define CARD_SERVICE_H

#include <memory>
#include <vector>
#include "model/Card.h"
#include "model/RequestResponse.h"
#include "model/Statistics.h"

namespace service {

class CardService {
public:
    virtual ~CardService() = default;
    
    // 创建卡片
    virtual model::ApiResponse<model::Card> createCard(int userId, const model::CreateCardRequest& request) = 0;
    
    // 获取卡片详情
    virtual model::ApiResponse<model::Card> getCardById(int userId, int cardId) = 0;
    
    // 更新卡片
    virtual model::ApiResponse<model::Card> updateCard(int userId, int cardId, const model::UpdateCardRequest& request) = 0;
    
    // 删除卡片
    virtual model::ApiResponse<void> deleteCard(int userId, int cardId) = 0;
    
    // 获取卡片列表
    virtual model::ApiResponse<model::PaginatedResponse<model::Card>> getCardList(int userId, const model::CardListQuery& query) = 0;
    
    // 按标签筛选卡片
    virtual model::ApiResponse<std::vector<model::Card>> getCardsByTags(int userId, const std::vector<std::string>& tags) = 0;
    
    // 搜索卡片
    virtual model::ApiResponse<model::PaginatedResponse<model::Card>> searchCards(int userId, const std::string& keyword, const model::CardListQuery& query) = 0;
    
    // 获取统计信息
    virtual model::ApiResponse<model::Statistics> getStatistics(int userId) = 0;
};

} // namespace service

#endif // CARD_SERVICE_H