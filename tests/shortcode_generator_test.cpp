#include <gtest/gtest.h>
#include "utils/shortcode_generator.h"

class ShortCodeGeneratorTest : public ::testing::Test {
protected:
    utils::ShortCodeGenerator generator;
};

TEST_F(ShortCodeGeneratorTest, GenerateRandomCodeDefaultLength) {
    // 测试默认长度的短码生成
    std::string code = generator.generateRandomCode();
    EXPECT_EQ(code.length(), 6);  // 默认长度为6
}

TEST_F(ShortCodeGeneratorTest, GenerateRandomCodeCustomLength) {
    // 测试自定义长度的短码生成
    std::string code = generator.generateRandomCode(8);
    EXPECT_EQ(code.length(), 8);
    
    code = generator.generateRandomCode(10);
    EXPECT_EQ(code.length(), 10);
}

TEST_F(ShortCodeGeneratorTest, GenerateRandomCodeZeroLength) {
    // 测试长度为0的情况，应该使用默认长度
    std::string code = generator.generateRandomCode(0);
    EXPECT_EQ(code.length(), 6);
}

TEST_F(ShortCodeGeneratorTest, GenerateRandomCodeNegativeLength) {
    // 测试负长度的情况，应该使用默认长度
    std::string code = generator.generateRandomCode(-5);
    EXPECT_EQ(code.length(), 6);
}

TEST_F(ShortCodeGeneratorTest, GeneratedCodesAreUnique) {
    // 测试生成的短码是否具有唯一性
    std::unordered_set<std::string> codes;
    const int numCodes = 1000;
    
    for (int i = 0; i < numCodes; ++i) {
        std::string code = generator.generateRandomCode();
        EXPECT_TRUE(codes.find(code) == codes.end());
        codes.insert(code);
    }
}

TEST_F(ShortCodeGeneratorTest, ValidateShortCode) {
    // 测试有效的短码
    EXPECT_TRUE(generator.isValidShortCode("abc123"));
    EXPECT_TRUE(generator.isValidShortCode("ABCdef"));
    EXPECT_TRUE(generator.isValidShortCode("abc_def"));
    EXPECT_TRUE(generator.isValidShortCode("abc-def"));
    
    // 测试无效的短码
    EXPECT_FALSE(generator.isValidShortCode(""));  // 空字符串
    EXPECT_FALSE(generator.isValidShortCode("abc"));  // 太短
    EXPECT_FALSE(generator.isValidShortCode("a"));  // 太短
    EXPECT_FALSE(generator.isValidShortCode("abc!123"));  // 包含非法字符
    EXPECT_FALSE(generator.isValidShortCode("abc@123"));  // 包含非法字符
}

TEST_F(ShortCodeGeneratorTest, ValidateShortCodeCustomLength) {
    // 测试自定义长度限制的短码验证
    EXPECT_TRUE(generator.isValidShortCode("abcd", 4, 10));  // 最小长度为4
    EXPECT_FALSE(generator.isValidShortCode("abc", 4, 10));  // 小于最小长度
    EXPECT_TRUE(generator.isValidShortCode("abcdefghij", 4, 10));  // 最大长度为10
    EXPECT_FALSE(generator.isValidShortCode("abcdefghijk", 4, 10));  // 大于最大长度
}

TEST_F(ShortCodeGeneratorTest, SetDefaultLength) {
    // 测试设置默认长度
    generator.setDefaultLength(8);
    EXPECT_EQ(generator.getDefaultLength(), 8);
    
    std::string code = generator.generateRandomCode();
    EXPECT_EQ(code.length(), 8);
    
    // 测试设置无效的默认长度
    generator.setDefaultLength(-5);
    EXPECT_EQ(generator.getDefaultLength(), 8);  // 应该保持不变
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
