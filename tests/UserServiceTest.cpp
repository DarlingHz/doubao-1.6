#include "include/service/UserService.h"
#include "include/service/UserServiceImpl.h"
#include "include/model/User.h"
#include <gtest/gtest.h>
#include <memory>

class UserServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<service::UserService> userService;

    void SetUp() override {
        // 创建UserService实例
        userService = std::make_shared<service::UserServiceImpl>();
    }

    void TearDown() override {
        // 清理资源
    }
};

// 测试密码加密功能
TEST_F(UserServiceTest, PasswordEncryptionTest) {
    std::string password = "testPassword123";
    std::string salt = "testSalt";
    
    // 测试加密密码
    std::string hashedPassword = userService->encryptPassword(password, salt);
    
    // 验证哈希结果不为空
    EXPECT_FALSE(hashedPassword.empty());
    
    // 验证相同密码和盐生成相同的哈希
    std::string hashedPassword2 = userService->encryptPassword(password, salt);
    EXPECT_EQ(hashedPassword, hashedPassword2);
    
    // 验证不同密码生成不同的哈希
    std::string hashedPassword3 = userService->encryptPassword("differentPassword", salt);
    EXPECT_NE(hashedPassword, hashedPassword3);
    
    // 验证不同盐生成不同的哈希
    std::string hashedPassword4 = userService->encryptPassword(password, "differentSalt");
    EXPECT_NE(hashedPassword, hashedPassword4);
}

// 测试密码验证功能
TEST_F(UserServiceTest, PasswordValidationTest) {
    std::string password = "testPassword123";
    std::string salt = "testSalt";
    
    // 加密密码
    std::string hashedPassword = userService->encryptPassword(password, salt);
    
    // 验证正确密码
    bool isValid = userService->validatePassword(password, hashedPassword, salt);
    EXPECT_TRUE(isValid);
    
    // 验证错误密码
    bool isInvalid = userService->validatePassword("wrongPassword", hashedPassword, salt);
    EXPECT_FALSE(isInvalid);
}

// 注意：以下测试需要实际的数据库连接，这里只是示例
// 在实际测试中，你可能需要设置测试数据库或使用模拟对象

// TEST_F(UserServiceTest, RegisterUserTest) {
//     model::RegisterRequest request;
//     request.email = "test@example.com";
//     request.password = "testPassword123";
//     
//     auto response = userService->registerUser(request);
//     
//     // 验证注册成功
//     EXPECT_EQ(response.code, 0);
//     EXPECT_FALSE(response.data.token.empty());
//     EXPECT_FALSE(response.data.userId == 0);
// }

// TEST_F(UserServiceTest, LoginTest) {
//     // 首先注册用户
//     model::RegisterRequest registerReq;
//     registerReq.email = "login_test@example.com";
//     registerReq.password = "testPassword123";
//     userService->registerUser(registerReq);
//     
//     // 然后尝试登录
//     model::LoginRequest loginReq;
//     loginReq.email = "login_test@example.com";
//     loginReq.password = "testPassword123";
//     
//     auto response = userService->login(loginReq);
//     
//     // 验证登录成功
//     EXPECT_EQ(response.code, 0);
//     EXPECT_FALSE(response.data.token.empty());
//     EXPECT_FALSE(response.data.userId == 0);
// }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}