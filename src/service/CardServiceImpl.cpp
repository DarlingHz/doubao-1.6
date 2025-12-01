#include "include/service/CardService.h"
#include "include/dao/CardDao.h"
#include "include/dao/TagDao.h"
#include "include/dao/CardDaoImpl.h"
#include "include/dao/TagDaoImpl.h"
#include "include/model/Card.h"
#include "include/model/RequestResponse.h"
#include "include/model/Statistics.h"
#include <chrono>

namespace service {

class CardServiceImpl : public CardService {
public:
    CardServiceImpl() {
        cardDao_ = std::make_shared<dao::CardDaoImpl>();
        tagDao_ = std::make_shared<dao::TagDaoImpl>();
    }
    
    model::ApiResponse<model::Card> createCard(int userId, const model::CreateCardRequest& request) override {
        model::ApiResponse<model::Card> response;
        
        // 验证输入
        if (request.title.empty()) {
            response.code = 5001;
            response.message = "标题不能为空";
            return response;
        }
        
        // 创建卡片对象
        auto now = std::chrono::system_clock::now();
        model::Card card(0, userId, request.title, request.content, request.tags, 
                        request.is_pinned, now, now);
        
        // 保存到数据库
        int cardId = cardDao_->create(card);
        if (cardId == 0) {
            response.code = 5002;
            response.message = "创建卡片失败";
            return response;
        }
        
        // 获取创建的卡片（包含ID等信息）
        auto createdCard = cardDao_->findById(cardId);
        if (!createdCard) {
            response.code = 5003;
            response.message = "创建卡片成功，但获取卡片信息失败";
            return response;
        }
        
        response.code = 0;
        response.message = "创建卡片成功";
        response.data = *createdCard;
        
        return response;
    }
    
    model::ApiResponse<model::Card> getCardById(int userId, int cardId) override {
        model::ApiResponse<model::Card> response;
        
        // 查询卡片
        auto card = cardDao_->findByUserIdAndCardId(userId, cardId);
        if (!card) {
            response.code = 6001;
            response.message = "卡片不存在或无权限访问";
            return response;
        }
        
        response.code = 0;
        response.message = "获取卡片成功";
        response.data = *card;
        
        return response;
    }
    
    model::ApiResponse<model::Card> updateCard(int userId, int cardId, const model::UpdateCardRequest& request) override {
        model::ApiResponse<model::Card> response;
        
        // 检查卡片是否存在且属于该用户
        auto existingCard = cardDao_->findByUserIdAndCardId(userId, cardId);
        if (!existingCard) {
            response.code = 6001;
            response.message = "卡片不存在或无权限访问";
            return response;
        }
        
        // 验证输入
        if (request.title.empty()) {
            response.code = 5001;
            response.message = "标题不能为空";
            return response;
        }
        
        // 更新卡片信息
        auto now = std::chrono::system_clock::now();
        model::Card updatedCard = *existingCard;
        updatedCard.setTitle(request.title);
        updatedCard.setContent(request.content);
        updatedCard.setTags(request.tags);
        updatedCard.setIsPinned(request.is_pinned);
        updatedCard.setUpdatedAt(now);
        
        // 保存更新
        if (!cardDao_->update(updatedCard)) {
            response.code = 7001;
            response.message = "更新卡片失败";
            return response;
        }
        
        // 获取更新后的卡片
        auto card = cardDao_->findById(cardId);
        if (!card) {
            response.code = 7002;
            response.message = "更新成功，但获取卡片信息失败";
            return response;
        }
        
        response.code = 0;
        response.message = "更新卡片成功";
        response.data = *card;
        
        return response;
    }
    
    model::ApiResponse<void> deleteCard(int userId, int cardId) override {
        model::ApiResponse<void> response;
        
        // 检查卡片是否存在且属于该用户
        auto existingCard = cardDao_->findByUserIdAndCardId(userId, cardId);
        if (!existingCard) {
            response.code = 6001;
            response.message = "卡片不存在或无权限访问";
            return response;
        }
        
        // 删除卡片
        if (!cardDao_->deleteById(cardId)) {
            response.code = 8001;
            response.message = "删除卡片失败";
            return response;
        }
        
        response.code = 0;
        response.message = "删除卡片成功";
        
        return response;
    }
    
    model::ApiResponse<model::PaginatedResponse<model::Card>> getCardList(int userId, const model::CardListQuery& query) override {
        model::ApiResponse<model::PaginatedResponse<model::Card>> response;
        
        // 查询卡片列表
        auto result = cardDao_->findByUserId(userId, query);
        
        response.code = 0;
        response.message = "获取卡片列表成功";
        response.data = result;
        
        return response;
    }
    
    model::ApiResponse<std::vector<model::Card>> getCardsByTags(int userId, const std::vector<std::string>& tags) override {
        model::ApiResponse<std::vector<model::Card>> response;
        
        if (tags.empty()) {
            response.code = 9001;
            response.message = "标签列表不能为空";
            return response;
        }
        
        // 查询卡片
        auto cards = cardDao_->findByTags(userId, tags);
        
        // 转换为vector<Card>
        std::vector<model::Card> cardList;
        for (const auto& card : cards) {
            cardList.push_back(*card);
        }
        
        response.code = 0;
        response.message = "按标签查询卡片成功";
        response.data = cardList;
        
        return response;
    }
    
    model::ApiResponse<model::PaginatedResponse<model::Card>> searchCards(int userId, const std::string& keyword, const model::CardListQuery& query) override {
        model::ApiResponse<model::PaginatedResponse<model::Card>> response;
        
        if (keyword.empty()) {
            response.code = 10001;
            response.message = "搜索关键字不能为空";
            return response;
        }
        
        // 执行搜索
        auto result = cardDao_->search(userId, keyword, query);
        
        response.code = 0;
        response.message = "搜索卡片成功";
        response.data = result;
        
        return response;
    }
    
    model::ApiResponse<model::Statistics> getStatistics(int userId) override {
        model::ApiResponse<model::Statistics> response;
        
        model::Statistics stats;
        
        // 获取总卡片数
        int totalCards = cardDao_->countByUserId(userId);
        stats.setTotalCards(totalCards);
        
        // 获取热门标签（Top 10）
        auto topTags = tagDao_->findTopTagsByUserId(userId, 10);
        std::vector<model::TagStatistics> tagStats;
        for (const auto& tag : topTags) {
            model::TagStatistics ts;
            ts.tagName = tag->getName();
            ts.count = tag->getCardCount();
            tagStats.push_back(ts);
        }
        stats.setTopTags(tagStats);
        
        // 获取最近7天的新增卡片数
        auto dailyCounts = cardDao_->getRecentDailyCount(userId, 7);
        std::vector<model::DailyCardCount> dailyCardCounts;
        for (const auto& [date, count] : dailyCounts) {
            model::DailyCardCount dcc;
            dcc.date = date;
            dcc.count = count;
            dailyCardCounts.push_back(dcc);
        }
        stats.setRecentDailyCounts(dailyCardCounts);
        
        response.code = 0;
        response.message = "获取统计信息成功";
        response.data = stats;
        
        return response;
    }
    
private:
    std::shared_ptr<dao::CardDao> cardDao_;
    std::shared_ptr<dao::TagDao> tagDao_;
};

} // namespace service