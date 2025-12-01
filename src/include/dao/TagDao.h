#ifndef TAG_DAO_H
#define TAG_DAO_H

#include <memory>
#include <vector>
#include <string>
#include "model/Tag.h"

namespace dao {

class TagDao {
public:
    virtual ~TagDao() = default;
    
    // 根据ID查找标签
    virtual std::shared_ptr<model::Tag> findById(int id) = 0;
    
    // 根据用户ID和标签名查找标签
    virtual std::shared_ptr<model::Tag> findByUserIdAndName(int userId, const std::string& name) = 0;
    
    // 获取用户的所有标签及其卡片数量
    virtual std::vector<std::shared_ptr<model::Tag>> findAllByUserId(int userId) = 0;
    
    // 创建新标签
    virtual int create(const model::Tag& tag) = 0;
    
    // 更新标签
    virtual bool update(const model::Tag& tag) = 0;
    
    // 删除标签
    virtual bool deleteById(int id) = 0;
    
    // 重命名标签
    virtual bool rename(int userId, const std::string& oldName, const std::string& newName) = 0;
    
    // 合并标签
    virtual bool merge(int userId, const std::string& tagToMerge, const std::string& targetTag) = 0;
    
    // 获取用户的Top N标签
    virtual std::vector<std::shared_ptr<model::Tag>> findTopTagsByUserId(int userId, int limit) = 0;
};

} // namespace dao

#endif // TAG_DAO_H