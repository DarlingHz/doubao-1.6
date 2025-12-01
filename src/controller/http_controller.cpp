// HTTP控制器基类实现
#include "http_controller.h"
#include "../utils/logger.h"
#include "../utils/stats_monitor.h"
#include <sstream>
#include <regex>

HttpController::HttpController() {
}

HttpController::~HttpController() {
}

void HttpController::registerRoute(const std::string& method, const std::string& pathPattern, RouteHandler handler) {
    routes.push_back({method, pathPattern, handler});
}

HttpResponse HttpController::handleRequest(const HttpRequest& request) {
    // 记录开始时间
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 查找匹配的路由
    for (const auto& route : routes) {
        if (route.method == request.method && matchPath(request.path, route.pathPattern)) {
            try {
                // 提取路径参数
                HttpRequest enrichedRequest = request;
                enrichedRequest.pathParams = extractPathParams(request.path, route.pathPattern);
                
                // 调用处理器
                HttpResponse response = route.handler(enrichedRequest);
                
                // 计算处理时间
                auto endTime = std::chrono::high_resolution_clock::now();
                long latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                
                // 记录延迟统计
                recordRequestLatency(request.path, latencyMs);
                
                Logger::getInstance()->debug(request.method + " " + request.path + " " + 
                                          std::to_string(response.statusCode) + " " + 
                                          std::to_string(latencyMs) + "ms");
                
                return response;
            } catch (const std::exception& e) {
                Logger::getInstance()->error("Exception handling request " + request.path + ": " + e.what());
                return createErrorResponse(500, "Internal server error: " + std::string(e.what()));
            } catch (...) {
                Logger::getInstance()->error("Unknown exception handling request " + request.path);
                return createErrorResponse(500, "Internal server error");
            }
        }
    }
    
    // 未找到路由
    return createErrorResponse(404, "Route not found: " + request.method + " " + request.path);
}

const std::vector<Route>& HttpController::getRoutes() const {
    return routes;
}

bool HttpController::matchPath(const std::string& path, const std::string& pattern) {
    // 简单的路径匹配算法
    // 将模式中的参数替换为正则表达式捕获组
    std::string regexPattern = pattern;
    std::regex paramRegex("\\:([a-zA-Z0-9_]+)");
    std::string replacement = "([^/]+)";
    std::string regexStr = "^" + std::regex_replace(regexPattern, paramRegex, replacement) + "$";
    
    std::regex regex(regexStr);
    return std::regex_match(path, regex);
}

std::unordered_map<std::string, std::string> HttpController::extractPathParams(const std::string& path, const std::string& pattern) {
    std::unordered_map<std::string, std::string> params;
    
    // 提取参数名和位置
    std::vector<std::string> paramNames;
    std::string processedPattern = pattern;
    std::regex paramRegex("\\:([a-zA-Z0-9_]+)");
    std::smatch match;
    
    std::string tempPattern = pattern;
    while (std::regex_search(tempPattern, match, paramRegex)) {
        paramNames.push_back(match[1]);
        tempPattern = match.suffix();
    }
    
    // 构建正则表达式来提取值
    std::string regexPattern = "^" + std::regex_replace(pattern, paramRegex, "([^/]+)") + "$";
    std::regex regex(regexPattern);
    
    if (std::regex_match(path, match, regex)) {
        // 匹配结果的第一个元素是整个匹配，之后是捕获组
        for (size_t i = 0; i < paramNames.size() && i + 1 < match.size(); i++) {
            params[paramNames[i]] = match[i + 1].str();
        }
    }
    
    return params;
}

void HttpController::recordRequestLatency(const std::string& endpoint, long latencyMs) {
    StatsMonitor::getInstance()->recordRequestLatency(endpoint, latencyMs);
    StatsMonitor::getInstance()->incrementTotalRequests();
}

HttpResponse HttpController::createSuccessResponse(const std::string& body) {
    HttpResponse response;
    response.statusCode = 200;
    response.body = body;
    response.headers["Content-Type"] = "application/json";
    return response;
}

HttpResponse HttpController::createErrorResponse(int statusCode, const std::string& message) {
    HttpResponse response;
    response.statusCode = statusCode;
    response.body = "{\"error\":\"" + message + "\"}";
    response.headers["Content-Type"] = "application/json";
    return response;
}

std::unordered_map<std::string, std::string> HttpController::parseJsonBody(const std::string& jsonBody) {
    std::unordered_map<std::string, std::string> result;
    
    // 简单的JSON解析（仅支持字符串值）
    // 注意：这是一个简化版本，实际项目中应使用专业的JSON库
    std::regex jsonRegex("\\\"([^\\\"]+)\\\":\\\"([^\\\"]*)\\\"");
    std::smatch match;
    std::string temp = jsonBody;
    
    while (std::regex_search(temp, match, jsonRegex)) {
        result[match[1]] = match[2];
        temp = match.suffix();
    }
    
    return result;
}

std::string HttpController::buildJsonResponse(const std::unordered_map<std::string, std::string>& data) {
    std::stringstream ss;
    ss << "{";
    
    bool first = true;
    for (const auto& [key, value] : data) {
        if (!first) {
            ss << ",";
        }
        first = false;
        ss << "\"" << key << "\":\"" << value << "\"";
    }
    
    ss << "}";
    return ss.str();
}

std::string HttpController::getParam(const HttpRequest& request, const std::string& paramName, const std::string& defaultValue) {
    // 优先从路径参数获取
    auto it = request.pathParams.find(paramName);
    if (it != request.pathParams.end()) {
        return it->second;
    }
    
    // 其次从查询参数获取
    it = request.queryParams.find(paramName);
    if (it != request.queryParams.end()) {
        return it->second;
    }
    
    return defaultValue;
}

int HttpController::getIntParam(const HttpRequest& request, const std::string& paramName, int defaultValue) {
    std::string paramValue = getParam(request, paramName);
    if (!paramValue.empty()) {
        try {
            return std::stoi(paramValue);
        } catch (...) {
            // 参数解析失败，返回默认值
        }
    }
    return defaultValue;
}

float HttpController::getFloatParam(const HttpRequest& request, const std::string& paramName, float defaultValue) {
    std::string paramValue = getParam(request, paramName);
    if (!paramValue.empty()) {
        try {
            return std::stof(paramValue);
        } catch (...) {
            // 参数解析失败，返回默认值
        }
    }
    return defaultValue;
}

bool HttpController::validateMethod(const HttpRequest& request, const std::string& expectedMethod) {
    return request.method == expectedMethod;
}
