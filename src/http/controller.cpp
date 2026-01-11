#include "../include/http/controller.h"
#include "../include/service/link_service.h"
#include "../include/model/link.h"
#include "../include/utils/logger.h"
#include "../include/utils/url_utils.h"
#include "../include/utils/time_utils.h"
#include <string>
#include <memory>

namespace http {

Controller::Controller(std::shared_ptr<service::LinkService> linkService)
    : linkService_(linkService) {
    utils::LOG_INFO("HTTP Controller initialized");
}

Controller::~Controller() {
    utils::LOG_INFO("HTTP Controller destroyed");
}

void Controller::registerRoutes(httplib::Server& server) {
    // API路由
    server.Post("/api/v1/shorten", [this](const httplib::Request& req, httplib::Response& res) {
        utils::LOG_INFO("Request received: POST /api/v1/shorten");
        this->handleCreateShortLink(req, res);
    });
    
    server.Get("/api/v1/links/:id/stats", [this](const httplib::Request& req, httplib::Response& res) {
        std::string logMsg = "Request received: GET /api/v1/links/";
        logMsg += req.matches[1].str();
        logMsg += "/stats";
        utils::LOG_INFO(logMsg);
        this->handleGetLinkStats(req, res);
    });
    
    server.Post("/api/v1/links/:id/disable", [this](const httplib::Request& req, httplib::Response& res) {
        std::string logMsg = "Request received: POST /api/v1/links/";
        logMsg += req.matches[1].str();
        logMsg += "/disable";
        utils::LOG_INFO(logMsg);
        this->handleDisableLink(req, res);
    });
    
    // 短链接重定向路由
    server.Get("/s/:short_code", [this](const httplib::Request& req, httplib::Response& res) {
        std::string logMsg = "Request received: GET /s/";
        logMsg += req.matches[1].str();
        utils::LOG_INFO(logMsg);
        this->handleRedirect(req, res);
    });
    
    // 健康检查
      server.Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
         utils::LOG_INFO("Request received: GET /health");
         this->handleHealthCheck(req, res);
     });
    
    // 404处理
    server.set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
        this->handleNotFound(req, res);
    });
    
    utils::LOG_INFO("All routes registered");
}

void Controller::handleCreateShortLink(const httplib::Request& req, httplib::Response& res) {
    try {
        // 解析JSON请求
        nlohmann::json requestBody;
        if (!parseJsonRequest(req, requestBody)) {
            sendErrorResponse(res, 400, "Invalid JSON format");
            return;
        }
        
        // 验证必需字段
        if (!requestBody.contains("long_url")) {
            sendErrorResponse(res, 400, "Missing required field: long_url");
            return;
        }
        
        // 构建请求对象
        model::CreateLinkRequest createRequest;
        createRequest.longUrl = requestBody["long_url"].get<std::string>();
        
        // 设置可选字段
        if (requestBody.contains("expire_seconds")) {
            createRequest.expireSeconds = requestBody["expire_seconds"].get<int>();
        }
        
        if (requestBody.contains("custom_alias")) {
            createRequest.customAlias = requestBody["custom_alias"].get<std::string>();
        }
        
        // 调用服务创建短链接
        auto response = linkService_->createShortLink(createRequest);
        
        if (response.success) {
            // 构建成功响应
            nlohmann::json responseData;
            responseData["success"] = true;
            responseData["message"] = response.message;
            responseData["data"] = {
                {"short_code", response.shortCode},
                {"full_short_url", response.fullShortUrl},
                {"expire_at", response.expireAt}
            };
            
            sendJsonResponse(res, 201, responseData);
        } else {
            // 构建失败响应
            sendErrorResponse(res, 400, response.message);
        }
    } catch (const std::exception& e) {
        utils::LOG_ERROR("Error handling create short link request: " + std::string(e.what()));
        sendErrorResponse(res, 500, "Internal server error");
    }
}

void Controller::handleRedirect(const httplib::Request& req, httplib::Response& res) {
    try {
        // 获取短码并转换为字符串
        std::string shortCode = req.matches[1].str();
        if (shortCode.empty()) {
            sendErrorResponse(res, 404, "Short code not found");
            return;
        }
        
        // 从服务获取短链接
        auto linkOpt = linkService_->getLinkByShortCode(shortCode);
        if (!linkOpt) {
            sendErrorResponse(res, 404, "Short link not found or expired");
            return;
        }
        
        const auto& link = *linkOpt;
        
        // 记录访问日志
        std::string clientIp = getClientIp(req);
        std::string userAgent = req.get_header_value("User-Agent");
        linkService_->logVisit(link.id, clientIp, userAgent);
        
        // 执行重定向
        res.set_redirect(link.longUrl, 302);  // 使用302临时重定向
        std::string logMsg = "Redirected: " + shortCode + " -> " + link.longUrl;
        utils::LOG_INFO(logMsg);
    } catch (const std::exception& e) {
        utils::LOG_ERROR("Error handling redirect: " + std::string(e.what()));
        sendErrorResponse(res, 500, "Internal server error");
    }
}

void Controller::handleGetLinkStats(const httplib::Request& req, httplib::Response& res) {
    try {
        // 获取链接ID并转换为字符串
        std::string idStr = req.matches[1].str();
        uint64_t linkId;
        
        try {
            linkId = std::stoull(idStr);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid link ID format");
            return;
        }
        
        // 获取统计信息
        auto statsOpt = linkService_->getLinkStats(linkId);
        if (!statsOpt) {
            sendErrorResponse(res, 404, "Link not found");
            return;
        }
        
        const auto& stats = *statsOpt;
        
        // 构建响应
        nlohmann::json responseData;
        responseData["success"] = true;
        responseData["data"] = {
            {"link_info", {
                {"id", stats.linkInfo.id},
                {"long_url", stats.linkInfo.longUrl},
                {"short_code", stats.linkInfo.shortCode},
                {"custom_alias", stats.linkInfo.customAlias},
                {"created_at", stats.linkInfo.createdAt},
                {"expire_at", stats.linkInfo.expireAt},
                {"status", static_cast<int>(stats.linkInfo.status)},
                {"visit_count", stats.linkInfo.visitCount}
            }},
            {"total_visits", stats.totalVisits}
        };
        
        // 添加最近访问记录
        nlohmann::json recentVisits = nlohmann::json::array();
        for (const auto& visit : stats.recentVisits) {
            recentVisits.push_back({
                {"id", visit.id},
                {"visit_time", visit.visitTime},
                {"ip_address", visit.ipAddress},
                {"user_agent", visit.userAgent}
            });
        }
        responseData["data"]["recent_visits"] = recentVisits;
        
        sendJsonResponse(res, 200, responseData);
    } catch (const std::exception& e) {
        utils::LOG_ERROR("Error handling get link stats: " + std::string(e.what()));
        sendErrorResponse(res, 500, "Internal server error");
    }
}

void Controller::handleDisableLink(const httplib::Request& req, httplib::Response& res) {
    try {
        // 获取链接ID并转换为字符串
        std::string idStr = req.matches[1].str();
        uint64_t linkId;
        
        try {
            linkId = std::stoull(idStr);
        } catch (...) {
            sendErrorResponse(res, 400, "Invalid link ID format");
            return;
        }
        
        // 禁用短链接
        utils::LOG_INFO("Disabling link with id: " + std::to_string(linkId));
        bool success = linkService_->disableLink(linkId);
        if (success) {
            nlohmann::json responseData;
            responseData["success"] = true;
            responseData["message"] = "Link disabled successfully";
            sendJsonResponse(res, 200, responseData);
        } else {
            sendErrorResponse(res, 404, "Link not found");
        }
    } catch (const std::exception& e) {
        utils::LOG_ERROR("Error handling disable link: " + std::string(e.what()));
        sendErrorResponse(res, 500, "Internal server error");
    }
}

void Controller::handleHealthCheck(const httplib::Request&, httplib::Response& res) {
    utils::LOG_INFO("Health check request received");
    nlohmann::json responseData;
    responseData["status"] = "ok";
    responseData["timestamp"] = utils::TimeUtils::getCurrentTimestamp();
    sendJsonResponse(res, 200, responseData);
}

void Controller::handleNotFound(const httplib::Request& req, httplib::Response& res) {
    utils::LOG_INFO("404 Not Found: " + req.path);
    sendErrorResponse(res, 404, "Endpoint not found");
}

void Controller::sendJsonResponse(httplib::Response& res, int statusCode, const nlohmann::json& data) {
    res.status = statusCode;
    res.set_content(data.dump(), "application/json");
}

void Controller::sendErrorResponse(httplib::Response& res, int statusCode, const std::string& message) {
    nlohmann::json errorData;
    errorData["success"] = false;
    errorData["error"] = message;
    sendJsonResponse(res, statusCode, errorData);
}

bool Controller::parseJsonRequest(const httplib::Request& req, nlohmann::json& outJson) {
    try {
        outJson = nlohmann::json::parse(req.body);
        return true;
    } catch (const nlohmann::json::exception& e) {
        utils::LOG_ERROR("Failed to parse JSON request: " + std::string(e.what()));
        return false;
    }
}

std::string Controller::getClientIp(const httplib::Request& req) {
    // 优先从X-Forwarded-For头部获取IP（如果有代理）
    std::string forwardedFor = req.get_header_value("X-Forwarded-For");
    if (!forwardedFor.empty()) {
        // X-Forwarded-For可能包含多个IP，取第一个
        size_t commaPos = forwardedFor.find(',');
        if (commaPos != std::string::npos) {
            return forwardedFor.substr(0, commaPos);
        }
        return forwardedFor;
    }
    
    // 从Remote-Addr获取
    return req.remote_addr;
}
} // namespace http
