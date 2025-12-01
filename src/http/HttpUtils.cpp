#include "HttpUtils.h"
#include <sstream>
#include <regex>

namespace http {
namespace utils {

std::string urlDecode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            char c = static_cast<char>(std::stoi(hex, nullptr, 16));
            decoded += c;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

std::string urlEncode(const std::string& decoded) {
    std::stringstream encoded;
    for (char c : decoded) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << "%" << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return encoded.str();
}

std::unordered_map<std::string, std::string> parseQueryParams(const std::string& query) {
    std::unordered_map<std::string, std::string> params;
    std::stringstream ss(query);
    std::string param;

    while (std::getline(ss, param, '&')) {
        size_t pos = param.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(param.substr(0, pos));
            std::string value = urlDecode(param.substr(pos + 1));
            params[key] = value;
        }
    }
    return params;
}

std::string formatTime(const std::string& timeStr) {
    // 简单的时间格式化，实际项目中可能需要更复杂的处理
    return timeStr;
}

std::string jsonEscape(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\"";
                break;
            case '\\': output += "\\\\";
                break;
            case '\b': output += "\\b";
                break;
            case '\f': output += "\\f";
                break;
            case '\n': output += "\\n";
                break;
            case '\r': output += "\\r";
                break;
            case '\t': output += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    output += "\\u";
                    char buf[5];
                    std::snprintf(buf, sizeof(buf), "%04x", static_cast<unsigned int>(c));
                    output += buf;
                } else {
                    output += c;
                }
                break;
        }
    }
    return output;
}

bool isValidHttpMethod(const std::string& method) {
    std::vector<std::string> validMethods = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"};
    for (const auto& validMethod : validMethods) {
        if (method == validMethod) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(str);
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace utils
} // namespace http