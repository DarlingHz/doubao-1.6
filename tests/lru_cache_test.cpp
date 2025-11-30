#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "storage/lru_cache.h"

class LRUCacheTest : public ::testing::Test {
protected:
    storage::LRUCache<std::string, int> cache{10, 0};  // 容量为10，默认永不过期
};

TEST_F(LRUCacheTest, BasicSetAndGet) {
    // 测试基本的设置和获取操作
    cache.set("key1", 1);
    cache.set("key2", 2);
    cache.set("key3", 3);
    
    auto value1 = cache.get("key1");
    auto value2 = cache.get("key2");
    auto value3 = cache.get("key3");
    
    EXPECT_TRUE(value1.has_value());
    EXPECT_EQ(value1.value(), 1);
    EXPECT_TRUE(value2.has_value());
    EXPECT_EQ(value2.value(), 2);
    EXPECT_TRUE(value3.has_value());
    EXPECT_EQ(value3.value(), 3);
}

TEST_F(LRUCacheTest, GetNonExistentKey) {
    // 测试获取不存在的键
    auto value = cache.get("non_existent_key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(LRUCacheTest, UpdateExistingKey) {
    // 测试更新已存在的键
    cache.set("key1", 1);
    cache.set("key1", 10);
    
    auto value = cache.get("key1");
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 10);
}

TEST_F(LRUCacheTest, LRUEviction) {
    // 测试LRU淘汰机制
    storage::LRUCache<std::string, int> smallCache(3, 0);  // 容量为3
    
    smallCache.set("key1", 1);
    smallCache.set("key2", 2);
    smallCache.set("key3", 3);
    
    // 访问key1，使其成为最近使用的
    smallCache.get("key1");
    
    // 添加新键，应该淘汰key2
    smallCache.set("key4", 4);
    
    EXPECT_TRUE(smallCache.get("key1").has_value());
    EXPECT_FALSE(smallCache.get("key2").has_value());  // key2应该被淘汰
    EXPECT_TRUE(smallCache.get("key3").has_value());
    EXPECT_TRUE(smallCache.get("key4").has_value());
}

TEST_F(LRUCacheTest, SizeAndCapacity) {
    // 测试缓存大小和容量
    EXPECT_EQ(cache.capacity(), 10);
    EXPECT_EQ(cache.size(), 0);
    
    cache.set("key1", 1);
    cache.set("key2", 2);
    
    EXPECT_EQ(cache.size(), 2);
    
    // 清空缓存
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
}

TEST_F(LRUCacheTest, SetCapacity) {
    // 测试设置容量
    cache.set("key1", 1);
    cache.set("key2", 2);
    cache.set("key3", 3);
    
    cache.setCapacity(2);
    EXPECT_EQ(cache.capacity(), 2);
    EXPECT_EQ(cache.size(), 2);
    
    // key1应该被淘汰
    EXPECT_FALSE(cache.get("key1").has_value());
    EXPECT_TRUE(cache.get("key2").has_value());
    EXPECT_TRUE(cache.get("key3").has_value());
}

TEST_F(LRUCacheTest, ItemExpiration) {
    // 测试项目过期
    storage::LRUCache<std::string, int> cacheWithTTL(10, 1);  // 容量10，默认TTL 1秒
    
    cacheWithTTL.set("key1", 1);
    cacheWithTTL.set("key2", 2, 3);  // 自定义TTL 3秒
    
    // 立即获取，应该都有效
    EXPECT_TRUE(cacheWithTTL.get("key1").has_value());
    EXPECT_TRUE(cacheWithTTL.get("key2").has_value());
    
    // 等待2秒，key1应该过期，key2仍然有效
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_FALSE(cacheWithTTL.get("key1").has_value());  // key1过期
    EXPECT_TRUE(cacheWithTTL.get("key2").has_value());  // key2仍然有效
    
    // 再等待2秒，key2也应该过期
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_FALSE(cacheWithTTL.get("key2").has_value());  // key2过期
}

TEST_F(LRUCacheTest, CleanupExpired) {
    // 测试清理过期项
    storage::LRUCache<std::string, int> cacheWithTTL(10, 1);  // 容量10，默认TTL 1秒
    
    cacheWithTTL.set("key1", 1);
    cacheWithTTL.set("key2", 2);
    cacheWithTTL.set("key3", 3, 10);  // 10秒后过期
    
    EXPECT_EQ(cacheWithTTL.size(), 3);
    
    // 等待2秒，key1和key2应该过期
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 清理过期项
    cacheWithTTL.cleanupExpired();
    
    EXPECT_EQ(cacheWithTTL.size(), 1);  // 只有key3保留
    EXPECT_FALSE(cacheWithTTL.get("key1").has_value());
    EXPECT_FALSE(cacheWithTTL.get("key2").has_value());
    EXPECT_TRUE(cacheWithTTL.get("key3").has_value());
}

TEST_F(LRUCacheTest, RemoveKey) {
    // 测试移除键
    cache.set("key1", 1);
    cache.set("key2", 2);
    
    EXPECT_EQ(cache.size(), 2);
    
    cache.remove("key1");
    EXPECT_EQ(cache.size(), 1);
    EXPECT_FALSE(cache.get("key1").has_value());
    EXPECT_TRUE(cache.get("key2").has_value());
    
    // 移除不存在的键
    cache.remove("non_existent_key");
    EXPECT_EQ(cache.size(), 1);
}

TEST_F(LRUCacheTest, ThreadSafety) {
    // 测试线程安全性
    storage::LRUCache<std::string, int> threadSafeCache(100, 0);
    std::atomic<int> errors{0};
    
    auto worker = [&]() {
        try {
            for (int i = 0; i < 1000; ++i) {
                std::string key = "key" + std::to_string(i % 100);
                threadSafeCache.set(key, i);
                auto value = threadSafeCache.get(key);
                if (value.has_value() && value.value() != i) {
                    errors++;
                }
            }
        } catch (...) {
            errors++;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(errors, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
