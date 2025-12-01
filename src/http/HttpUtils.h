#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include <string>
#include <unordered_map>
#include <vector>

namespace http {

namespace utils {

std::string urlDecode(const std::string& encoded);
std::string urlEncode(const std::string& decoded);
std::unordered_map<std::string, std::string> parseQueryParams(const std::string& query);
std::string formatTime(const std::string& timeStr);
std::string jsonEscape(const std::string& input);
bool isValidHttpMethod(const std::string& method);
std::vector<std::string> split(const std::string& str, char delimiter);

} // namespace utils

} // namespace http

#endif // HTTPUTILS_H