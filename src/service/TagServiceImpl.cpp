#include "include/service/TagService.h"
#include "include/dao/TagDao.h"
#include "include/dao/TagDaoImpl.h"
#include "include/model/Tag.h"
#include "include/model/RequestResponse.h"

namespace service {

class TagServiceImpl : public TagService {
public:
    TagServiceImpl() {
        tagDao_ = std::make_shared<dao::TagDaoImpl>();
    }
    
    model::ApiResponse<std::vector<model::Tag>> getUserTags(int userId) override {
        model::ApiResponse<std::vector<model::Tag>> response;
        
        // 获取用户所有标签
        auto tags = tagDao_->findAllByUserId(userId);
        
        // 转换为vector<Tag>
        std::vector<model::Tag> tagList;
        for (const auto& tag : tags) {
            tagList.push_back(*tag);
        }
        
        response.code = 0;
        response.message = "获取标签列表成功";
        response.data = tagList;
        
        return response;
    }
    
    model::ApiResponse<void> renameTag(int userId, const std::string& oldName, const std::string& newName) override {
        model::ApiResponse<void> response;
        
        // 验证输入
        if (oldName.empty() || newName.empty()) {
            response.code = 11001;
            response.message = "标签名称不能为空";
            return response;
        }
        
        if (oldName == newName) {
            response.code = 11002;
            response.message = "新旧标签名称相同";
            return response;
        }
        
        // 检查旧标签是否存在
        auto oldTag = tagDao_->findByUserIdAndName(userId, oldName);
        if (!oldTag) {
            response.code = 11003;
            response.message = "旧标签不存在";
            return response;
        }
        
        // 执行重命名
        if (!tagDao_->rename(userId, oldName, newName)) {
            response.code = 11004;
            response.message = "重命名标签失败，可能新标签名已存在";
            return response;
        }
        
        response.code = 0;
        response.message = "重命名标签成功";
        
        return response;
    }
    
    model::ApiResponse<void> mergeTags(int userId, const std::string& tagToMerge, const std::string& targetTag) override {
        model::ApiResponse<void> response;
        
        // 验证输入
        if (tagToMerge.empty() || targetTag.empty()) {
            response.code = 11001;
            response.message = "标签名称不能为空";
            return response;
        }
        
        if (tagToMerge == targetTag) {
            response.code = 11005;
            response.message = "不能合并标签到自身";
            return response;
        }
        
        // 检查标签是否存在
        auto mergeTag = tagDao_->findByUserIdAndName(userId, tagToMerge);
        auto targetTagObj = tagDao_->findByUserIdAndName(userId, targetTag);
        
        if (!mergeTag) {
            response.code = 11006;
            response.message = "要合并的标签不存在";
            return response;
        }
        
        if (!targetTagObj) {
            response.code = 11007;
            response.message = "目标标签不存在";
            return response;
        }
        
        // 执行合并
        if (!tagDao_->merge(userId, tagToMerge, targetTag)) {
            response.code = 11008;
            response.message = "合并标签失败";
            return response;
        }
        
        response.code = 0;
        response.message = "合并标签成功";
        
        return response;
    }
    
    model::ApiResponse<std::vector<model::Tag>> getTopTags(int userId, int limit) override {
        model::ApiResponse<std::vector<model::Tag>> response;
        
        // 验证参数
        if (limit <= 0 || limit > 100) {
            limit = 10; // 默认返回前10个
        }
        
        // 获取热门标签
        auto tags = tagDao_->findTopTagsByUserId(userId, limit);
        
        // 转换为vector<Tag>
        std::vector<model::Tag> tagList;
        for (const auto& tag : tags) {
            tagList.push_back(*tag);
        }
        
        response.code = 0;
        response.message = "获取热门标签成功";
        response.data = tagList;
        
        return response;
    }
    
private:
    std::shared_ptr<dao::TagDao> tagDao_;
};

} // namespace service