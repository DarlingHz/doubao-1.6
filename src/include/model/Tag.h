#ifndef TAG_H
#define TAG_H

#include <string>

namespace model {

class Tag {
public:
    Tag() = default;
    
    Tag(int id, int userId, const std::string& name, int cardCount = 0)
        : id_(id), userId_(userId), name_(name), cardCount_(cardCount) {}
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    int getUserId() const { return userId_; }
    void setUserId(int userId) { userId_ = userId; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    int getCardCount() const { return cardCount_; }
    void setCardCount(int cardCount) { cardCount_ = cardCount; }
    
private:
    int id_ = 0;
    int userId_ = 0;
    std::string name_;
    int cardCount_ = 0; // 该标签关联的卡片数量
};

} // namespace model

#endif // TAG_H