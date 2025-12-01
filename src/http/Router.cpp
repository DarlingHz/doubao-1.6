#include "Router.h"
#include "RequestHandler.h"
#include <sstream>
#include <regex>

namespace http {

void Router::addRoute(const std::string& method, const std::string& path, Handler handler) {
    routes.push_back({method, path, handler});
}

bool Router::route(const HttpRequest& request, HttpResponse& response) {
    for (const auto& route : routes) {
        if (route.method != request.method) {
            continue;
        }

        std::map<std::string, std::string> params;
        if (matchPath(request.path, route.path, params)) {
            // 保存路径参数到请求对象中
            const_cast<HttpRequest&>(request).pathParams = params;
            route.handler(request, response);
            return true;
        }
    }
    return false;
}

bool Router::matchPath(const std::string& path, const std::string& routePath, std::map<std::string, std::string>& params) const {
    // 将路由路径转换为正则表达式
    std::string regexPath = routePath;
    std::regex paramRegex("\\\\:([\\\\w]+)");
    std::string replacement = "([\\\\w\\\\-]+)";
    std::string pattern = std::regex_replace(regexPath, paramRegex, replacement);
    pattern = "^" + pattern + "$";

    std::regex regex(pattern);
    std::smatch match;
    if (std::regex_match(path, match, regex)) {
        // 提取路径参数
        std::vector<std::string> paramNames;
        std::string::const_iterator searchStart(regexPath.cbegin());
        std::smatch paramMatch;
        while (std::regex_search(searchStart, regexPath.cend(), paramMatch, paramRegex)) {
            paramNames.push_back(paramMatch[1]);
            searchStart = paramMatch.suffix().first;
        }

        // 保存参数值
        for (size_t i = 1; i < match.size(); ++i) {
            if (i - 1 < paramNames.size()) {
                params[paramNames[i - 1]] = match[i].str();
            }
        }
        return true;
    }
    return false;
}

std::map<std::string, std::string> Router::extractPathParams(const std::string& path, const std::string& routePath) const {
    std::map<std::string, std::string> params;
    matchPath(path, routePath, params);
    return params;
}

} // namespace http