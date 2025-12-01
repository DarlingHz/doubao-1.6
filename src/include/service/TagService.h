#ifndef TAG_SERVICE_H
#define TAG_SERVICE_H

#include <memory>
#include <vector>
#include "model/Tag.h"
#include "model/RequestResponse.h"

namespace service {

class TagService {
public:
    virtual ~TagService() = default;
    
    // 获取用户的所有标签
    virtual model::ApiResponse<std::vector<model::Tag>> getUserTags(int userId) = 0;
    
    // 重命名标签
    virtual model::ApiResponse<void> renameTag(int userId, const std::string& oldName, const std::string& newName) = 0;
    
    // 合并标签
    virtual model::ApiResponse<void> mergeTags(int userId, const std::string& tagToMerge, const std::string& targetTag) = 0;
    
    // 获取用户的热门标签
    virtual model::ApiResponse<std::vector<model::Tag>> getTopTags(int userId, int limit = 10) = 0;
};

} // namespace service

#endif // TAG_SERVICE_H