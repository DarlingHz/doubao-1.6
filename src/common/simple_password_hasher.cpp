#include "simple_password_hasher.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <functional>

SimplePasswordHasher::SimplePasswordHasher() {
    // 初始化盐值生成器
    std::random_device rd;
    salt_generator_.seed(rd());
}

std::string SimplePasswordHasher::HashPassword(const std::string& password) {
    // 生成随机盐值
    std::string salt = GenerateSalt();
    
    // 计算密码哈希值
    std::string hash = ComputeHash(password + salt);
    
    // 返回盐值和哈希值的组合
    return salt + ":" + hash;
}

bool SimplePasswordHasher::VerifyPassword(const std::string& password, const std::string& hashed_password) {
    // 从哈希密码中提取盐值和哈希值
    size_t colon_pos = hashed_password.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string salt = hashed_password.substr(0, colon_pos);
    std::string hash = hashed_password.substr(colon_pos + 1);
    
    // 计算输入密码的哈希值
    std::string computed_hash = ComputeHash(password + salt);
    
    // 比较计算得到的哈希值和存储的哈希值
    return computed_hash == hash;
}

std::string SimplePasswordHasher::GenerateSalt() {
    // 生成16字节的随机盐值
    std::uniform_int_distribution<int> dist(0, 255);
    std::string salt;
    
    for (int i = 0; i < 16; ++i) {
        salt += static_cast<char>(dist(salt_generator_));
    }
    
    // 将盐值转换为十六进制字符串
    return ToHexString(salt);
}

std::string SimplePasswordHasher::ComputeHash(const std::string& input) {
    // 使用C++标准库中的哈希函数
    std::hash<std::string> hash_func;
    size_t hash_value = hash_func(input);
    
    // 将哈希值转换为十六进制字符串
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash_value;
    
    return ss.str();
}

std::string SimplePasswordHasher::ToHexString(const std::string& input) {
    std::stringstream ss;
    
    for (char c : input) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
    }
    
    return ss.str();
}