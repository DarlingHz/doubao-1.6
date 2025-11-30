#pragma once

#include <string>
#include <regex>

namespace utils {

class UrlUtils {
public:
    // 验证URL格式是否有效
    static bool isValidUrl(const std::string& url) {
        // 基本的URL正则表达式，支持http、https、ftp等协议
        const std::regex urlRegex(
            R"(^(https?|ftp)://)"  // 协议部分
            R"(([a-zA-Z0-9_-]+(?:\.[a-zA-Z0-9_-]+)+))"  // 域名部分
            R"(:?([0-9]+))?"  // 端口部分（可选）
            R"(/?[^\s]*)"  // 路径部分（可选）
            R"($)");
        return std::regex_match(url, urlRegex);
    }
    
    // 构建短链接URL
    static std::string buildShortUrl(const std::string& baseUrl, const std::string& shortCode) {
        return baseUrl + "/s/" + shortCode;
    }
    
    // 验证自定义别名是否有效
    static bool isValidCustomAlias(const std::string& alias, size_t maxLength) {
        // 自定义别名只能包含字母、数字、下划线和连字符，且长度不超过maxLength
        const std::regex aliasRegex("^[a-zA-Z0-9_-]+$");
        return !alias.empty() && alias.length() <= maxLength && std::regex_match(alias, aliasRegex);
    }
};
} // namespace utils