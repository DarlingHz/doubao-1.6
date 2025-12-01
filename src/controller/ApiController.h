#ifndef API_CONTROLLER_H
#define API_CONTROLLER_H

#include <string>
#include <memory>
#include "include/service/UserService.h"
#include "include/service/CardService.h"
#include "include/service/TagService.h"

namespace controller {

class ApiController {
public:
    ApiController();
    
    // 用户相关接口
    std::string handleRegister(const std::string& requestBody);
    std::string handleLogin(const std::string& requestBody);
    
    // 卡片相关接口
    std::string handleCreateCard(const std::string& requestBody, int userId);
    std::string handleGetCard(const std::string& cardId, int userId);
    std::string handleUpdateCard(const std::string& cardId, const std::string& requestBody, int userId);
    std::string handleDeleteCard(const std::string& cardId, int userId);
    std::string handleGetCardList(const std::string& queryString, int userId);
    std::string handleSearchCards(const std::string& queryString, int userId);
    
    // 标签相关接口
    std::string handleGetTags(int userId);
    std::string handleRenameTag(const std::string& requestBody, int userId);
    std::string handleMergeTags(const std::string& requestBody, int userId);
    std::string handleGetTopTags(int userId, const std::string& limitStr);
    
    // 统计接口
    std::string handleGetStatistics(int userId);
    
    // 辅助方法：从请求头获取token并验证
    int validateAuthToken(const std::string& authHeader);
    
private:
    std::shared_ptr<service::UserService> userService_;
    std::shared_ptr<service::CardService> cardService_;
    std::shared_ptr<service::TagService> tagService_;
    
    // 解析查询参数
    std::map<std::string, std::string> parseQueryString(const std::string& queryString);
    
    // 构建错误响应
    std::string buildErrorResponse(int code, const std::string& message);
};

} // namespace controller

#endif // API_CONTROLLER_H