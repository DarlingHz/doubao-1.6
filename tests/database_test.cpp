#include <gtest/gtest.h>
#include <string>
#include "storage/database.h"
#include "model/link.h"

class DatabaseTest : public ::testing::Test {
protected:
    std::shared_ptr<storage::Database> db;
    
    void SetUp() override {
        // 使用内存数据库进行测试
        db = std::make_shared<storage::Database>(":memory:");
        ASSERT_TRUE(db->initialize());
    }
    
    void TearDown() override {
        // 测试完成后自动清理
    }
    
    // 创建测试用的短链接
    model::ShortLink createTestLink() {
        model::ShortLink link;
        link.longUrl = "https://example.com/test/long/url";
        link.shortCode = "test123";
        link.customAlias = "test-alias";
        link.createdAt = time(nullptr);
        link.expireAt = 0;  // 永不过期
        link.status = model::LinkStatus::ACTIVE;
        link.visitCount = 0;
        return link;
    }
};

TEST_F(DatabaseTest, CreateAndGetLink) {
    // 测试创建和获取短链接
    auto link = createTestLink();
    
    // 创建短链接
    auto linkIdOpt = db->createLink(link);
    ASSERT_TRUE(linkIdOpt.has_value());
    uint64_t linkId = linkIdOpt.value();
    
    // 通过ID获取短链接
    auto retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    
    auto retrievedLink = retrievedLinkOpt.value();
    EXPECT_EQ(retrievedLink.id, linkId);
    EXPECT_EQ(retrievedLink.longUrl, link.longUrl);
    EXPECT_EQ(retrievedLink.shortCode, link.shortCode);
    EXPECT_EQ(retrievedLink.customAlias, link.customAlias);
    EXPECT_EQ(retrievedLink.createdAt, link.createdAt);
    EXPECT_EQ(retrievedLink.expireAt, link.expireAt);
    EXPECT_EQ(retrievedLink.status, link.status);
    EXPECT_EQ(retrievedLink.visitCount, link.visitCount);
}

TEST_F(DatabaseTest, GetLinkByShortCode) {
    // 测试通过短码获取短链接
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    auto retrievedLinkOpt = db->getLinkByShortCode(link.shortCode);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().id, linkId);
    
    // 测试获取不存在的短链接
    auto nonExistent = db->getLinkByShortCode("non-existent");
    EXPECT_FALSE(nonExistent.has_value());
}

TEST_F(DatabaseTest, GetLinkByCustomAlias) {
    // 测试通过自定义别名获取短链接
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    auto retrievedLinkOpt = db->getLinkByCustomAlias(link.customAlias);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().id, linkId);
    
    // 测试获取不存在的自定义别名
    auto nonExistent = db->getLinkByCustomAlias("non-existent");
    EXPECT_FALSE(nonExistent.has_value());
}

TEST_F(DatabaseTest, UpdateLink) {
    // 测试更新短链接
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    // 修改短链接
    link.id = linkId;
    link.longUrl = "https://example.com/updated/url";
    link.status = model::LinkStatus::DISABLED;
    link.visitCount = 10;
    
    bool updated = db->updateLink(link);
    ASSERT_TRUE(updated);
    
    // 验证更新
    auto retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    
    auto retrievedLink = retrievedLinkOpt.value();
    EXPECT_EQ(retrievedLink.longUrl, link.longUrl);
    EXPECT_EQ(retrievedLink.status, link.status);
    EXPECT_EQ(retrievedLink.visitCount, link.visitCount);
}

TEST_F(DatabaseTest, DisableLink) {
    // 测试禁用短链接
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    bool disabled = db->disableLink(linkId);
    ASSERT_TRUE(disabled);
    
    // 验证禁用状态
    auto retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().status, model::LinkStatus::DISABLED);
    
    // 测试禁用不存在的短链接
    bool disableNonExistent = db->disableLink(999999);
    EXPECT_FALSE(disableNonExistent);
}

TEST_F(DatabaseTest, LogVisit) {
    // 测试记录访问日志
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    // 记录访问日志
    model::VisitLog log;
    log.linkId = linkId;
    log.visitTime = time(nullptr);
    log.ipAddress = "127.0.0.1";
    log.userAgent = "Test Agent";
    
    bool logged = db->logVisit(log);
    ASSERT_TRUE(logged);
    
    // 验证访问计数增加
    auto retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().visitCount, 1);
    
    // 记录多次访问
    db->logVisit(log);
    db->logVisit(log);
    
    // 再次验证访问计数
    retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().visitCount, 3);
}

TEST_F(DatabaseTest, GetLinkStats) {
    // 测试获取短链接统计信息
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    // 记录几次访问
    for (int i = 0; i < 5; ++i) {
        model::VisitLog log;
        log.linkId = linkId;
        log.visitTime = time(nullptr) + i;
        log.ipAddress = "127.0.0." + std::to_string(i + 1);
        log.userAgent = "Test Agent " + std::to_string(i);
        db->logVisit(log);
    }
    
    // 获取统计信息
    auto statsOpt = db->getLinkStats(linkId, 3);
    ASSERT_TRUE(statsOpt.has_value());
    
    auto stats = statsOpt.value();
    EXPECT_EQ(stats.linkInfo.id, linkId);
    EXPECT_EQ(stats.totalVisits, 5);
    EXPECT_EQ(stats.recentVisits.size(), 3);  // 只返回最近3次
    
    // 验证访问日志按时间倒序排列
    if (stats.recentVisits.size() >= 2) {
        EXPECT_GE(stats.recentVisits[0].visitTime, stats.recentVisits[1].visitTime);
    }
}

TEST_F(DatabaseTest, CheckExistence) {
    // 测试检查短码和自定义别名是否存在
    auto link = createTestLink();
    db->createLink(link);
    
    // 检查存在的短码和自定义别名
    EXPECT_TRUE(db->isShortCodeExists(link.shortCode));
    EXPECT_TRUE(db->isCustomAliasExists(link.customAlias));
    
    // 检查不存在的短码和自定义别名
    EXPECT_FALSE(db->isShortCodeExists("non-existent-code"));
    EXPECT_FALSE(db->isCustomAliasExists("non-existent-alias"));
}

TEST_F(DatabaseTest, IncrementVisitCount) {
    // 测试增加访问计数
    auto link = createTestLink();
    auto linkId = db->createLink(link).value();
    
    bool incremented = db->incrementVisitCount(linkId);
    ASSERT_TRUE(incremented);
    
    auto retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().visitCount, 1);
    
    // 多次增加
    db->incrementVisitCount(linkId);
    db->incrementVisitCount(linkId);
    
    retrievedLinkOpt = db->getLinkById(linkId);
    ASSERT_TRUE(retrievedLinkOpt.has_value());
    EXPECT_EQ(retrievedLinkOpt.value().visitCount, 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
