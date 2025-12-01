#include "include/service/UserService.h"
#include "include/dao/UserDao.h"
#include "include/dao/UserDaoImpl.h"
#include "include/model/User.h"
#include "include/model/RequestResponse.h"
#include <chrono>
#include <random>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <base64.h> // 假设使用一个base64库
#include <nlohmann/json.hpp> // 假设使用nlohmann/json库

namespace service {

class UserServiceImpl : public UserService {
public:
    UserServiceImpl() {
        userDao_ = std::make_shared<dao::UserDaoImpl>();
        jwtSecret_ = "your_secret_key_here"; // 在实际应用中应该从配置文件读取
        jwtExpiration_ = 24 * 60 * 60; // 24小时
    }
    
    model::ApiResponse<model::LoginResponse> registerUser(const model::RegisterRequest& request) override {
        model::ApiResponse<model::LoginResponse> response;
        
        // 检查邮箱是否已存在
        auto existingUser = userDao_->findByEmail(request.email);
        if (existingUser) {
            response.code = 1001;
            response.message = "邮箱已被注册";
            return response;
        }
        
        // 验证密码长度
        if (request.password.length() < 6) {
            response.code = 1002;
            response.message = "密码长度至少为6位";
            return response;
        }
        
        // 加密密码
        auto [passwordHash, salt] = hashPassword(request.password);
        
        // 创建用户
        auto now = std::chrono::system_clock::now();
        model::User user(0, request.email, passwordHash, salt, now, now);
        
        int userId = userDao_->create(user);
        if (userId == 0) {
            response.code = 1003;
            response.message = "注册失败，请稍后重试";
            return response;
        }
        
        // 生成token
        std::string token = generateToken(userId, request.email);
        
        // 构造响应
        model::LoginResponse loginResponse;
        loginResponse.token = token;
        loginResponse.userId = userId;
        loginResponse.email = request.email;
        
        response.code = 0;
        response.message = "注册成功";
        response.data = loginResponse;
        
        return response;
    }
    
    model::ApiResponse<model::LoginResponse> login(const model::LoginRequest& request) override {
        model::ApiResponse<model::LoginResponse> response;
        
        // 查找用户
        auto user = userDao_->findByEmail(request.email);
        if (!user) {
            response.code = 2001;
            response.message = "邮箱或密码错误";
            return response;
        }
        
        // 验证密码
        if (!verifyPassword(request.password, user->getPasswordHash(), user->getSalt())) {
            response.code = 2001;
            response.message = "邮箱或密码错误";
            return response;
        }
        
        // 生成token
        std::string token = generateToken(user->getId(), user->getEmail());
        
        // 构造响应
        model::LoginResponse loginResponse;
        loginResponse.token = token;
        loginResponse.userId = user->getId();
        loginResponse.email = user->getEmail();
        
        response.code = 0;
        response.message = "登录成功";
        response.data = loginResponse;
        
        return response;
    }
    
    model::ApiResponse<model::User> getUserById(int id) override {
        model::ApiResponse<model::User> response;
        
        auto user = userDao_->findById(id);
        if (!user) {
            response.code = 3001;
            response.message = "用户不存在";
            return response;
        }
        
        response.code = 0;
        response.message = "获取用户信息成功";
        response.data = *user;
        
        return response;
    }
    
    model::ApiResponse<int> validateToken(const std::string& token) override {
        model::ApiResponse<int> response;
        
        try {
            // 解码JWT
            auto parts = splitToken(token);
            if (parts.size() != 3) {
                response.code = 4001;
                response.message = "无效的token格式";
                return response;
            }
            
            // 验证签名
            if (!verifyTokenSignature(parts[0] + "." + parts[1], parts[2])) {
                response.code = 4002;
                response.message = "无效的token签名";
                return response;
            }
            
            // 解析payload
            nlohmann::json payload = nlohmann::json::parse(base64Decode(parts[1]));
            
            // 验证过期时间
            auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000;
            if (payload.contains("exp") && now > payload["exp"]) {
                response.code = 4003;
                response.message = "token已过期";
                return response;
            }
            
            // 提取用户ID
            int userId = payload["userId"];
            
            response.code = 0;
            response.message = "token验证成功";
            response.data = userId;
            
        } catch (const std::exception& e) {
            response.code = 4004;
            response.message = "token解析失败";
        }
        
        return response;
    }
    
    std::string generateToken(int userId, const std::string& email) override {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::seconds(jwtExpiration_);
        
        // 创建JWT payload
        nlohmann::json payload;
        payload["userId"] = userId;
        payload["email"] = email;
        payload["iat"] = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        payload["exp"] = std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count();
        
        // 创建JWT header
        nlohmann::json header;
        header["alg"] = "HS256";
        header["typ"] = "JWT";
        
        // Base64编码
        std::string encodedHeader = base64Encode(header.dump());
        std::string encodedPayload = base64Encode(payload.dump());
        
        // 生成签名
        std::string signatureData = encodedHeader + "." + encodedPayload;
        std::string signature = hmacSha256(signatureData, jwtSecret_);
        std::string encodedSignature = base64Encode(signature);
        
        // 组合JWT
        return encodedHeader + "." + encodedPayload + "." + encodedSignature;
    }
    
    std::pair<std::string, std::string> hashPassword(const std::string& password) override {
        // 生成随机盐
        std::string salt = generateRandomSalt();
        
        // 计算密码哈希 (SHA-256)
        std::string data = password + salt;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
        
        // 转换为十六进制字符串
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        
        return {ss.str(), salt};
    }
    
    bool verifyPassword(const std::string& password, const std::string& passwordHash, const std::string& salt) override {
        // 使用相同的盐计算密码哈希
        std::string data = password + salt;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
        
        // 转换为十六进制字符串
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        
        // 比较哈希值
        return ss.str() == passwordHash;
    }
    
private:
    std::shared_ptr<dao::UserDao> userDao_;
    std::string jwtSecret_;
    int jwtExpiration_; // 单位：秒
    
    std::string generateRandomSalt() {
        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        const int saltLength = 32;
        
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, charset.size() - 1);
        
        std::string salt;
        for (int i = 0; i < saltLength; i++) {
            salt += charset[distribution(generator)];
        }
        
        return salt;
    }
    
    std::vector<std::string> splitToken(const std::string& token) {
        std::vector<std::string> parts;
        std::stringstream ss(token);
        std::string part;
        
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }
        
        return parts;
    }
    
    bool verifyTokenSignature(const std::string& data, const std::string& signature) {
        std::string expectedSignature = hmacSha256(data, jwtSecret_);
        std::string decodedSignature = base64Decode(signature);
        
        return expectedSignature == decodedSignature;
    }
    
    std::string hmacSha256(const std::string& data, const std::string& key) {
        unsigned char* digest = HMAC(EVP_sha256(), 
                                    key.c_str(), key.length(),
                                    reinterpret_cast<const unsigned char*>(data.c_str()), 
                                    data.length(), NULL, NULL);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << digest[i];
        }
        
        return ss.str();
    }
    
    std::string base64Encode(const std::string& input) {
        // 这里需要实现base64编码，或者使用第三方库
        // 简单示例，实际应用中应使用成熟的base64库
        return "encoded_base64_string"; // 占位符
    }
    
    std::string base64Decode(const std::string& input) {
        // 这里需要实现base64解码，或者使用第三方库
        // 简单示例，实际应用中应使用成熟的base64库
        return "decoded_string"; // 占位符
    }
};

} // namespace service