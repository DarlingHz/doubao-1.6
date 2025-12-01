#include "include/controller/ApiController.h"
#include "include/service/UserServiceImpl.h"
#include "include/service/CardServiceImpl.h"
#include "include/service/TagServiceImpl.h"
#include "include/model/RequestResponse.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace controller {

ApiController::ApiController() {
    userService_ = std::make_shared<service::UserServiceImpl>();
    cardService_ = std::make_shared<service::CardServiceImpl>();
    tagService_ = std::make_shared<service::TagServiceImpl>();
}

std::string ApiController::handleRegister(const std::string& requestBody) {
    try {
        auto json = nlohmann::json::parse(requestBody);
        
        model::RegisterRequest request;
        request.email = json["email"];
        request.password = json["password"];
        
        auto response = userService_->registerUser(request);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            result["data"]["token"] = response.data.token;
            result["data"]["userId"] = response.data.userId;
            result["data"]["email"] = response.data.email;
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "注册请求处理失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleLogin(const std::string& requestBody) {
    try {
        auto json = nlohmann::json::parse(requestBody);
        
        model::LoginRequest request;
        request.email = json["email"];
        request.password = json["password"];
        
        auto response = userService_->login(request);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            result["data"]["token"] = response.data.token;
            result["data"]["userId"] = response.data.userId;
            result["data"]["email"] = response.data.email;
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "登录请求处理失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleCreateCard(const std::string& requestBody, int userId) {
    try {
        auto json = nlohmann::json::parse(requestBody);
        
        model::CreateCardRequest request;
        request.title = json["title"];
        request.content = json.value("content", "");
        request.is_pinned = json.value("is_pinned", false);
        
        if (json.contains("tags")) {
            for (const auto& tag : json["tags"]) {
                request.tags.push_back(tag);
            }
        }
        
        auto response = cardService_->createCard(userId, request);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& card = response.data;
            result["data"]["id"] = card.getId();
            result["data"]["title"] = card.getTitle();
            result["data"]["content"] = card.getContent();
            result["data"]["tags"] = card.getTags();
            result["data"]["is_pinned"] = card.isPinned();
            // 可以添加时间字段
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "创建卡片失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleGetCard(const std::string& cardId, int userId) {
    try {
        int id = std::stoi(cardId);
        auto response = cardService_->getCardById(userId, id);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& card = response.data;
            result["data"]["id"] = card.getId();
            result["data"]["title"] = card.getTitle();
            result["data"]["content"] = card.getContent();
            result["data"]["tags"] = card.getTags();
            result["data"]["is_pinned"] = card.isPinned();
            // 可以添加时间字段
        }
        
        return result.dump();
    } catch (const std::invalid_argument&) {
        return buildErrorResponse(400, "无效的卡片ID");
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "获取卡片失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleUpdateCard(const std::string& cardId, const std::string& requestBody, int userId) {
    try {
        int id = std::stoi(cardId);
        auto json = nlohmann::json::parse(requestBody);
        
        model::UpdateCardRequest request;
        request.title = json["title"];
        request.content = json.value("content", "");
        request.is_pinned = json.value("is_pinned", false);
        
        if (json.contains("tags")) {
            for (const auto& tag : json["tags"]) {
                request.tags.push_back(tag);
            }
        }
        
        auto response = cardService_->updateCard(userId, id, request);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& card = response.data;
            result["data"]["id"] = card.getId();
            result["data"]["title"] = card.getTitle();
            result["data"]["content"] = card.getContent();
            result["data"]["tags"] = card.getTags();
            result["data"]["is_pinned"] = card.isPinned();
            // 可以添加时间字段
        }
        
        return result.dump();
    } catch (const std::invalid_argument&) {
        return buildErrorResponse(400, "无效的卡片ID");
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "更新卡片失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleDeleteCard(const std::string& cardId, int userId) {
    try {
        int id = std::stoi(cardId);
        auto response = cardService_->deleteCard(userId, id);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        return result.dump();
    } catch (const std::invalid_argument&) {
        return buildErrorResponse(400, "无效的卡片ID");
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "删除卡片失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleGetCardList(const std::string& queryString, int userId) {
    try {
        auto params = parseQueryString(queryString);
        model::CardListQuery query;
        
        if (params.count("page")) query.page = std::stoi(params["page"]);
        if (params.count("page_size")) query.page_size = std::stoi(params["page_size"]);
        if (params.count("sort_by")) query.sort_by = params["sort_by"];
        if (params.count("sort_order")) query.sort_order = params["sort_order"];
        
        auto response = cardService_->getCardList(userId, query);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& data = response.data;
            result["data"]["total"] = data.total;
            result["data"]["page"] = data.page;
            result["data"]["page_size"] = data.page_size;
            
            for (const auto& card : data.items) {
                nlohmann::json cardJson;
                cardJson["id"] = card.getId();
                cardJson["title"] = card.getTitle();
                cardJson["tags"] = card.getTags();
                cardJson["is_pinned"] = card.isPinned();
                // 可以添加时间字段
                result["data"]["items"].push_back(cardJson);
            }
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "获取卡片列表失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleSearchCards(const std::string& queryString, int userId) {
    try {
        auto params = parseQueryString(queryString);
        model::CardListQuery query;
        
        if (params.count("page")) query.page = std::stoi(params["page"]);
        if (params.count("page_size")) query.page_size = std::stoi(params["page_size"]);
        if (params.count("sort_by")) query.sort_by = params["sort_by"];
        if (params.count("sort_order")) query.sort_order = params["sort_order"];
        
        std::string keyword = params.value("keyword", "");
        
        auto response = cardService_->searchCards(userId, keyword, query);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& data = response.data;
            result["data"]["total"] = data.total;
            result["data"]["page"] = data.page;
            result["data"]["page_size"] = data.page_size;
            
            for (const auto& card : data.items) {
                nlohmann::json cardJson;
                cardJson["id"] = card.getId();
                cardJson["title"] = card.getTitle();
                cardJson["tags"] = card.getTags();
                cardJson["is_pinned"] = card.isPinned();
                // 可以添加时间字段
                result["data"]["items"].push_back(cardJson);
            }
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "搜索卡片失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleGetTags(int userId) {
    try {
        auto response = tagService_->getUserTags(userId);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            for (const auto& tag : response.data) {
                nlohmann::json tagJson;
                tagJson["id"] = tag.getId();
                tagJson["name"] = tag.getName();
                tagJson["card_count"] = tag.getCardCount();
                result["data"].push_back(tagJson);
            }
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "获取标签列表失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleRenameTag(const std::string& requestBody, int userId) {
    try {
        auto json = nlohmann::json::parse(requestBody);
        std::string oldName = json["old_name"];
        std::string newName = json["new_name"];
        
        auto response = tagService_->renameTag(userId, oldName, newName);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "重命名标签失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleMergeTags(const std::string& requestBody, int userId) {
    try {
        auto json = nlohmann::json::parse(requestBody);
        std::string tagToMerge = json["tag_to_merge"];
        std::string targetTag = json["target_tag"];
        
        auto response = tagService_->mergeTags(userId, tagToMerge, targetTag);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "合并标签失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleGetTopTags(int userId, const std::string& limitStr) {
    try {
        int limit = 10;
        if (!limitStr.empty()) {
            limit = std::stoi(limitStr);
        }
        
        auto response = tagService_->getTopTags(userId, limit);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            for (const auto& tag : response.data) {
                nlohmann::json tagJson;
                tagJson["id"] = tag.getId();
                tagJson["name"] = tag.getName();
                tagJson["card_count"] = tag.getCardCount();
                result["data"].push_back(tagJson);
            }
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "获取热门标签失败: " + std::string(e.what()));
    }
}

std::string ApiController::handleGetStatistics(int userId) {
    try {
        auto response = cardService_->getStatistics(userId);
        
        nlohmann::json result;
        result["code"] = response.code;
        result["message"] = response.message;
        
        if (response.code == 0) {
            const auto& stats = response.data;
            result["data"]["total_cards"] = stats.getTotalCards();
            
            for (const auto& tagStat : stats.getTopTags()) {
                nlohmann::json tagJson;
                tagJson["tag_name"] = tagStat.tagName;
                tagJson["count"] = tagStat.count;
                result["data"]["top_tags"].push_back(tagJson);
            }
            
            for (const auto& dailyCount : stats.getRecentDailyCounts()) {
                nlohmann::json dailyJson;
                dailyJson["date"] = dailyCount.date;
                dailyJson["count"] = dailyCount.count;
                result["data"]["recent_daily_counts"].push_back(dailyJson);
            }
        }
        
        return result.dump();
    } catch (const std::exception& e) {
        return buildErrorResponse(500, "获取统计信息失败: " + std::string(e.what()));
    }
}

int ApiController::validateAuthToken(const std::string& authHeader) {
    // 从Authorization头中提取token
    // 格式：Bearer <token>
    if (authHeader.substr(0, 7) != "Bearer ") {
        return -1;
    }
    
    std::string token = authHeader.substr(7);
    auto response = userService_->validateToken(token);
    
    if (response.code != 0) {
        return -1;
    }
    
    return response.data;
}

std::map<std::string, std::string> ApiController::parseQueryString(const std::string& queryString) {
    std::map<std::string, std::string> params;
    std::stringstream ss(queryString);
    std::string pair;
    
    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            // 简单的URL解码
            // 实际应用中应该使用更完善的URL解码函数
            params[key] = value;
        }
    }
    
    return params;
}

std::string ApiController::buildErrorResponse(int code, const std::string& message) {
    nlohmann::json result;
    result["code"] = code;
    result["message"] = message;
    return result.dump();
}

} // namespace controller