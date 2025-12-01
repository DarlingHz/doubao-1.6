#ifndef MEMORY_CACHE_HPP
#define MEMORY_CACHE_HPP

#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <functional>
#include "utils/utils.hpp"

namespace api_quota {
namespace utils {

// 简单的LRU缓存实现
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class MemoryCache {
public:
    struct CacheEntry {
        Value value;
        std::chrono::steady_clock::time_point last_accessed;
        size_t access_count;
        std::chrono::steady_clock::time_point expiry;
    };

    explicit MemoryCache(size_t capacity = 1000, std::chrono::seconds default_ttl = std::chrono::seconds(3600))
        : capacity_(capacity), default_ttl_(default_ttl) {}

    // 获取缓存值，如果不存在则调用加载函数
    Value get_or_load(const Key& key, std::function<Value()> load_func, 
                      std::optional<std::chrono::seconds> ttl = std::nullopt) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end() && !is_expired(it)) {
            // 更新访问时间
            it->second.last_accessed = std::chrono::steady_clock::now();
            it->second.access_count++;
            return it->second.value;
        }
        
        // 加载新值
        Value value = load_func();
        put(key, value, ttl);
        return value;
    }

    // 直接设置缓存值
    void put(const Key& key, const Value& value, 
             std::optional<std::chrono::seconds> ttl = std::nullopt) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 清理过期项
        cleanup_expired();
        
        // 如果容量已满，移除最少使用的项
        if (cache_.size() >= capacity_ && cache_.find(key) == cache_.end()) {
            evict_lru();
        }
        
        CacheEntry entry;
        entry.value = value;
        entry.last_accessed = std::chrono::steady_clock::now();
        entry.access_count = 0;
        
        if (ttl.has_value()) {
            entry.expiry = entry.last_accessed + ttl.value();
        } else {
            entry.expiry = entry.last_accessed + default_ttl_;
        }
        
        cache_[key] = entry;
    }

    // 获取缓存值（不加载）
    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end() && !is_expired(it)) {
            it->second.last_accessed = std::chrono::steady_clock::now();
            it->second.access_count++;
            return it->second.value;
        }
        
        return std::nullopt;
    }

    // 移除缓存项
    void remove(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    // 清除所有缓存
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

    // 获取缓存大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

private:
    size_t capacity_;
    std::chrono::seconds default_ttl_;
    std::unordered_map<Key, CacheEntry, Hash> cache_;
    mutable std::mutex mutex_;

    bool is_expired(typename std::unordered_map<Key, CacheEntry, Hash>::iterator it) {
        return std::chrono::steady_clock::now() > it->second.expiry;
    }

    void cleanup_expired() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (now > it->second.expiry) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void evict_lru() {
        if (cache_.empty()) return;
        
        auto lru_it = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.last_accessed < lru_it->second.last_accessed) {
                lru_it = it;
            }
        }
        
        cache_.erase(lru_it);
    }
};

// 计数器缓存，用于存储API调用计数
template<typename Key>
class CounterCache {
public:
    explicit CounterCache(size_t shard_count = 16)
        : shard_count_(shard_count), shards_(shard_count) {}

    // 增加计数
    int64_t increment(const Key& key, int64_t delta = 1) {
        size_t shard_index = get_shard_index(key);
        auto& [map, mutex] = shards_[shard_index];
        std::lock_guard<std::mutex> lock(mutex);
        return map[key] += delta;
    }

    // 获取计数
    int64_t get(const Key& key) const {
        size_t shard_index = get_shard_index(key);
        auto& [map, mutex] = shards_[shard_index];
        std::lock_guard<std::mutex> lock(mutex);
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return 0;
    }

    // 重置计数
    void reset(const Key& key) {
        size_t shard_index = get_shard_index(key);
        auto& [map, mutex] = shards_[shard_index];
        std::lock_guard<std::mutex> lock(mutex);
        map.erase(key);
    }

    // 批量重置计数
    void reset_batch(const std::vector<Key>& keys) {
        for (const auto& key : keys) {
            reset(key);
        }
    }

    // 获取所有计数（用于持久化）
    std::unordered_map<Key, int64_t> get_all() const {
        std::unordered_map<Key, int64_t> result;
        
        for (const auto& [map, mutex] : shards_) {
            std::lock_guard<std::mutex> lock(mutex);
            for (const auto& [key, value] : map) {
                result[key] += value;
            }
        }
        
        return result;
    }

private:
    struct Shard {
        std::unordered_map<Key, int64_t> map;
        mutable std::mutex mutex;
    };

    size_t shard_count_;
    std::vector<Shard> shards_;

    size_t get_shard_index(const Key& key) const {
        return hash_string(key) % shard_count_;
    }

    // 简单的哈希函数
    size_t hash_string(const Key& key) const {
        std::hash<Key> hasher;
        return hasher(key);
    }
};

} // namespace utils
} // namespace api_quota

#endif // MEMORY_CACHE_HPP