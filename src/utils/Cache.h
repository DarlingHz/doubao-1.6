#pragma once
#include <unordered_map>
#include <mutex>
#include <optional>
#include <string>
#include <chrono>

namespace accounting {

// 简单的线程安全缓存类
template <typename K, typename V>
class Cache {
public:
    Cache(size_t maxSize = 100, int expireSeconds = 3600) 
        : maxSize_(maxSize), expireSeconds_(expireSeconds) {}
    
    // 获取缓存项
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            // 检查是否过期
            auto now = std::chrono::system_clock::now();
            if (expireSeconds_ > 0 && 
                std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count() > expireSeconds_) {
                // 过期，删除缓存
                cache_.erase(it);
                return std::nullopt;
            }
            
            return it->second.value;
        }
        
        return std::nullopt;
    }
    
    // 添加缓存项
    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 检查容量，移除最旧的项
        if (cache_.size() >= maxSize_ && cache_.find(key) == cache_.end()) {
            removeOldest();
        }
        
        cache_[key] = CacheItem{value, std::chrono::system_clock::now()};
    }
    
    // 删除缓存项
    void remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }
    
    // 清空缓存
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
    
    // 获取缓存大小
    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }
    
private:
    struct CacheItem {
        V value;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::unordered_map<K, CacheItem> cache_;
    mutable std::mutex mutex_;
    size_t maxSize_;
    int expireSeconds_; // 缓存过期时间（秒），0表示永不过期
    
    // 移除最旧的缓存项
    void removeOldest() {
        if (cache_.empty()) {
            return;
        }
        
        auto oldestIt = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.timestamp < oldestIt->second.timestamp) {
                oldestIt = it;
            }
        }
        
        cache_.erase(oldestIt);
    }
};

} // namespace accounting
