#include "utils/Cache.h"
#include <iostream>

namespace utils {

Cache& Cache::getInstance() {
    static Cache instance;
    return instance;
}

Cache::Cache() : maxCacheSize(1000) {
    lastCleanup = std::chrono::system_clock::now();
}

Cache::~Cache() {
}

void Cache::set(const std::string& key, const std::string& value, std::optional<std::chrono::seconds> expiry) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // 定期清理过期项目
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::minutes>(now - lastCleanup).count() >= 5) {
        cleanupExpiredItems();
        lastCleanup = now;
    }
    
    // 如果缓存已满，清理一些项目
    if (cache.size() >= maxCacheSize) {
        // 简单策略：删除5%的最旧项目
        size_t itemsToRemove = cache.size() / 20;
        auto it = cache.begin();
        while (itemsToRemove > 0 && it != cache.end()) {
            if (isExpired(it->second)) {
                it = cache.erase(it);
                --itemsToRemove;
            } else {
                ++it;
            }
        }
        
        // 如果没有足够的过期项目，删除一些旧项目
        if (itemsToRemove > 0) {
            size_t removed = 0;
            for (auto it = cache.begin(); it != cache.end() && removed < itemsToRemove; ) {
                it = cache.erase(it);
                ++removed;
            }
        }
    }
    
    CacheItem item;
    item.value = value;
    item.hasExpiry = expiry.has_value();
    if (expiry.has_value()) {
        item.expiryTime = std::chrono::system_clock::now() + expiry.value();
    }
    
    cache[key] = item;
}

std::optional<std::string> Cache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        if (isExpired(it->second)) {
            cache.erase(it);
            return std::nullopt;
        }
        return it->second.value;
    }
    return std::nullopt;
}

void Cache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.erase(key);
}

void Cache::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
}

size_t Cache::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.size();
}

void Cache::setMaxSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    maxCacheSize = maxSize;
    
    // 如果当前缓存大小超过新的最大值，清理一些项目
    if (cache.size() > maxSize) {
        cleanupExpiredItems();
        
        // 如果仍然超过，删除一些项目
        while (cache.size() > maxSize) {
            auto it = cache.begin();
            if (it != cache.end()) {
                cache.erase(it);
            }
        }
    }
}

void Cache::cleanupExpiredItems() {
    for (auto it = cache.begin(); it != cache.end(); ) {
        if (isExpired(it->second)) {
            it = cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool Cache::isExpired(const CacheItem& item) const {
    if (!item.hasExpiry) {
        return false;
    }
    return std::chrono::system_clock::now() > item.expiryTime;
}

} // namespace utils