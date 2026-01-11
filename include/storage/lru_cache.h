#pragma once

#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>
#include <chrono>
#include <iostream>

namespace storage {

// LRU缓存项，包含值和过期时间
template <typename T>
struct CacheItem {
    T value;
    uint64_t expireTime;  // 过期时间戳（秒），0表示永不过期
    
    bool isExpired() const {
        if (expireTime == 0) {
            return false;  // 永不过期
        }
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return now > expireTime;
    }
};

// LRU缓存实现
template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity, uint64_t defaultTTL = 0)
        : capacity_(capacity), defaultTTL_(defaultTTL) {}

    // 设置缓存项
    void set(const K& key, const V& value, uint64_t ttl = 0) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 计算过期时间
        uint64_t expireTime = 0;
        if (ttl > 0 || defaultTTL_ > 0) {
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            expireTime = now + (ttl > 0 ? ttl : defaultTTL_);
        }

        // 如果key已存在，先移除旧的
        if (cacheMap_.find(key) != cacheMap_.end()) {
            cacheList_.erase(cacheMap_[key]);
            cacheMap_.erase(key);
        }

        // 如果缓存已满，删除最久未使用的项
        if (cacheMap_.size() >= capacity_) {
            auto& last = cacheList_.back();
            cacheMap_.erase(last.first);
            cacheList_.pop_back();
        }

        // 添加新项到头部
        cacheList_.push_front({key, {value, expireTime}});
        cacheMap_[key] = cacheList_.begin();
    }

    // 获取缓存项
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            auto& cacheItem = it->second->second;
            
            // 检查是否过期
            if (cacheItem.isExpired()) {
                // 移除过期项
                cacheList_.erase(it->second);
                cacheMap_.erase(it);
                return std::nullopt;
            }

            // 移动到头部（最近使用）
            cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
            
            return cacheItem.value;
        }

        return std::nullopt;
    }

    // 删除缓存项
    void remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cacheMap_.find(key);
        if (it != cacheMap_.end()) {
            cacheList_.erase(it->second);
            cacheMap_.erase(it);
        }
    }

    // 清空缓存
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cacheList_.clear();
        cacheMap_.clear();
    }

    // 获取缓存大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cacheMap_.size();
    }

    // 获取缓存容量
    size_t capacity() const {
        return capacity_;
    }

    // 设置缓存容量
    void setCapacity(size_t capacity) {
        std::lock_guard<std::mutex> lock(mutex_);
        capacity_ = capacity;
        
        // 清理超出容量的项
        while (cacheMap_.size() > capacity_) {
            auto& last = cacheList_.back();
            cacheMap_.erase(last.first);
            cacheList_.pop_back();
        }
    }

    // 清理过期项
    void cleanupExpired() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cacheList_.begin();
        while (it != cacheList_.end()) {
            if (it->second.isExpired()) {
                cacheMap_.erase(it->first);
                it = cacheList_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    size_t capacity_;  // 缓存容量
    uint64_t defaultTTL_;  // 默认过期时间（秒）
    
    // 缓存列表，按使用顺序排列，最近使用的在头部
    using CacheList = std::list<std::pair<K, CacheItem<V>>>;
    CacheList cacheList_;
    
    // 哈希表，用于快速查找缓存项在列表中的位置
    std::unordered_map<K, typename CacheList::iterator> cacheMap_;
    
    // 互斥锁，保证线程安全
    mutable std::mutex mutex_;
};

} // namespace storage
