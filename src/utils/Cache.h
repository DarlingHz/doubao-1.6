#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <optional>

namespace utils {

struct CacheItem {
    std::string value;
    std::chrono::system_clock::time_point expiryTime;
    bool hasExpiry;
};

class Cache {
public:
    static Cache& getInstance();

    void set(const std::string& key, const std::string& value, std::optional<std::chrono::seconds> expiry = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    void remove(const std::string& key);
    void clear();
    size_t size() const;
    void setMaxSize(size_t maxSize);

private:
    Cache();
    ~Cache();

    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    void cleanupExpiredItems();
    bool isExpired(const CacheItem& item) const;

    std::unordered_map<std::string, CacheItem> cache;
    mutable std::mutex cacheMutex;
    size_t maxCacheSize;
    std::chrono::system_clock::time_point lastCleanup;
};

} // namespace utils

#endif // CACHE_H