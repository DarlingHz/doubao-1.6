#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>
#include <chrono>

namespace model {

class Card {
public:
    Card() = default;
    
    Card(int id, int userId, const std::string& title, const std::string& content,
         const std::vector<std::string>& tags, bool isPinned,
         const std::chrono::system_clock::time_point& createdAt,
         const std::chrono::system_clock::time_point& updatedAt)
        : id_(id), userId_(userId), title_(title), content_(content), tags_(tags),
          isPinned_(isPinned), createdAt_(createdAt), updatedAt_(updatedAt) {}
    
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }
    
    int getUserId() const { return userId_; }
    void setUserId(int userId) { userId_ = userId; }
    
    const std::string& getTitle() const { return title_; }
    void setTitle(const std::string& title) { title_ = title; }
    
    const std::string& getContent() const { return content_; }
    void setContent(const std::string& content) { content_ = content; }
    
    const std::vector<std::string>& getTags() const { return tags_; }
    void setTags(const std::vector<std::string>& tags) { tags_ = tags; }
    
    bool isPinned() const { return isPinned_; }
    void setIsPinned(bool isPinned) { isPinned_ = isPinned; }
    
    const std::chrono::system_clock::time_point& getCreatedAt() const { return createdAt_; }
    void setCreatedAt(const std::chrono::system_clock::time_point& createdAt) { createdAt_ = createdAt; }
    
    const std::chrono::system_clock::time_point& getUpdatedAt() const { return updatedAt_; }
    void setUpdatedAt(const std::chrono::system_clock::time_point& updatedAt) { updatedAt_ = updatedAt; }
    
private:
    int id_ = 0;
    int userId_ = 0;
    std::string title_;
    std::string content_;
    std::vector<std::string> tags_;
    bool isPinned_ = false;
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point updatedAt_;
};

} // namespace model

#endif // CARD_H