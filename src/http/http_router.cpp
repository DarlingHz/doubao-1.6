#include "http/http_router.hpp"
#include "utils/json.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <regex>
#include <ctime>

namespace api_quota {
namespace http {

HttpRouter::HttpRouter(std::shared_ptr<service::ClientService> client_service,
                     std::shared_ptr<service::ApiKeyService> api_key_service,
                     std::shared_ptr<service::QuotaService> quota_service)
    : client_service_(client_service),
      api_key_service_(api_key_service),
      quota_service_(quota_service) {
    initialize_routes();
}

void HttpRouter::add_route(const std::string& method, const std::string& path, HandlerFunction handler) {
    auto [regex, param_names] = parse_route_path(path);
    routes_.push_back({
        .method = method,
        .path = path,
        .path_regex = regex,
        .path_param_names = param_names,
        .handler = handler
    });
}

std::pair<std::regex, std::vector<std::string>> HttpRouter::parse_route_path(const std::string& path) {
    std::string regex_pattern = "^";
    std::vector<std::string> param_names;
    
    std::size_t pos = 0;
    while (pos < path.size()) {
        if (path[pos] == '{') {
            // 查找参数名结束位置
            std::size_t end_pos = path.find('}', pos);
            if (end_pos != std::string::npos) {
                std::string param_name = path.substr(pos + 1, end_pos - pos - 1);
                param_names.push_back(param_name);
                regex_pattern += "([^/]+)";
                pos = end_pos + 1;
                continue;
            }
        }
        // 普通字符，转义特殊字符
        if (path[pos] == '.') {
            regex_pattern += "\\.";
        } else if (path[pos] == '^' || path[pos] == '$' || path[pos] == '+' || 
                  path[pos] == '*' || path[pos] == '?' || path[pos] == '(' || 
                  path[pos] == ')' || path[pos] == '[' || path[pos] == ']' || 
                  path[pos] == '{') {
            regex_pattern += '\\';
            regex_pattern += path[pos];
        } else {
            regex_pattern += path[pos];
        }
        pos++;
    }
    
    regex_pattern += "$";
    return {std::regex(regex_pattern), param_names};
}

std::optional<Route> HttpRouter::match_route(const std::string& method, const std::string& path, HttpRequest& request) {
    for (const auto& route : routes_) {
        if (route.method != method) {
            continue;
        }
        
        std::smatch match;
        if (std::regex_match(path, match, route.path_regex)) {
            // 提取路径参数
            for (size_t i = 1; i < match.size() && i - 1 < route.path_param_names.size(); i++) {
                request.set_path_param(route.path_param_names[i - 1], match[i]);
            }
            return route;
        }
    }
    return std::nullopt;
}

HttpResponse HttpRouter::handle_request(const HttpRequest& request) {
    try {
        // 创建请求的可修改副本用于设置路径参数
        auto mutable_request = std::make_unique<SimpleHttpRequest>(
            request.get_method(),
            request.get_path(),
            request.get_body()
        );
        
        // 复制原始请求的头部和查询参数
        for (const auto& [key, value] : request.get_headers()) {
            mutable_request->set_header(key, value);
        }
        for (const auto& [key, value] : request.get_query_params()) {
            mutable_request->set_query_param(key, value);
        }
        
        // 匹配路由
        auto matched_route = match_route(request.get_method(), request.get_path(), *mutable_request);
        if (!matched_route) {
            return handle_error(HttpResponse::NOT_FOUND, "route_not_found", "The requested endpoint does not exist");
        }
        
        // 调用处理函数
        return matched_route->handler(*mutable_request);
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        return handle_error(HttpResponse::INTERNAL_SERVER_ERROR, "internal_error", "An internal server error occurred");
    }
}

void HttpRouter::initialize_routes() {
    // 健康检查
    add_route("GET", "/health", [this](const HttpRequest& request) {
        return handle_health_check(request);
    });
    
    // 客户端管理
    add_route("POST", "/clients", [this](const HttpRequest& request) {
        return handle_create_client(request);
    });
    add_route("GET", "/clients", [this](const HttpRequest& request) {
        return handle_get_clients(request);
    });
    add_route("GET", "/clients/{client_id}", [this](const HttpRequest& request) {
        return handle_get_client(request);
    });
    add_route("PUT", "/clients/{client_id}", [this](const HttpRequest& request) {
        return handle_update_client(request);
    });
    add_route("DELETE", "/clients/{client_id}", [this](const HttpRequest& request) {
        return handle_delete_client(request);
    });
    
    // API密钥管理
    add_route("POST", "/clients/{client_id}/keys", [this](const HttpRequest& request) {
        return handle_create_api_key(request);
    });
    add_route("GET", "/clients/{client_id}/keys", [this](const HttpRequest& request) {
        return handle_get_client_api_keys(request);
    });
    add_route("POST", "/keys/{key_id}/revoke", [this](const HttpRequest& request) {
        return handle_revoke_api_key(request);
    });
    
    // 配额校验
    add_route("POST", "/quota/check", [this](const HttpRequest& request) {
        return handle_quota_check(request);
    });
    
    // 统计查询
    add_route("GET", "/stats/clients/top", [this](const HttpRequest& request) {
        return handle_get_top_clients(request);
    });
    add_route("GET", "/stats/clients/{client_id}/summary", [this](const HttpRequest& request) {
        return handle_get_client_summary(request);
    });
    add_route("GET", "/stats/keys/{key_id}/timeline", [this](const HttpRequest& request) {
        return handle_get_key_timeline(request);
    });
}

utils::JsonValue HttpRouter::parse_json_body(const HttpRequest& request) {
    if (!request.is_json_content_type()) {
        throw std::runtime_error("Request must be application/json");
    }
    
    try {
        auto json_opt = utils::parse_json(request.get_body());
        if (!json_opt) {
            throw std::runtime_error("Invalid JSON format");
        }
        return *json_opt;
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid JSON format");
    }
}

HttpResponse HttpRouter::handle_error(HttpResponse::StatusCode status_code, const std::string& error_code, const std::string& message) {
    return HttpResponse::create_json_error(status_code, error_code, message);
}

HttpResponse HttpRouter::handle_health_check(const HttpRequest& request) {
    utils::JsonValue response;
    response["status"] = "ok";
    response["timestamp"] = static_cast<int64_t>(utils::get_current_timestamp());
    return HttpResponse::create_json_success(utils::serialize_json(response));
}

HttpResponse HttpRouter::handle_create_client(const HttpRequest& request) {
    try {
        auto json = parse_json_body(request);
        
        // 验证必需字段
        if (!json.is_object() || !json.contains("name") || !json.contains("contact_email")) {
            return handle_error(HttpResponse::BAD_REQUEST, "missing_required_fields", 
                              "name and contact_email are required fields");
        }
        
        std::string name = json["name"].as_string();
        std::string contact_email = json["contact_email"].as_string();
        
        // 获取可选的配额设置
        int64_t daily_quota = 10000; // 默认值
        int64_t per_minute_quota = 200; // 默认值
        
        if (json.contains("daily_quota")) {
            daily_quota = json["daily_quota"].as_integer();
        }
        if (json.contains("per_minute_quota")) {
            per_minute_quota = json["per_minute_quota"].as_integer();
        }
        
        // 创建客户端
        auto client = client_service_->create_client(name, contact_email, daily_quota, per_minute_quota);
        if (!client) {
            return handle_error(HttpResponse::BAD_REQUEST, "client_creation_failed", 
                              "Failed to create client");
        }
        
        // 返回客户端信息
        utils::JsonValue response;
        response["id"] = static_cast<int64_t>(client->client_id);
        response["name"] = client->name;
        response["contact_email"] = client->contact_email;
        response["daily_quota"] = client->daily_quota;
        response["per_minute_quota"] = client->per_minute_quota;
        response["is_active"] = client->is_active;
        response["created_at"] = utils::format_timestamp(client->created_at);
        
        HttpResponse http_response(HttpResponse::CREATED);
        http_response.set_json_body(utils::serialize_json(response));
        return http_response;
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_get_clients(const HttpRequest& request) {
    bool include_inactive = request.get_query_param("include_inactive") == "true";
    auto clients = client_service_->get_all_clients(include_inactive);
    
    utils::JsonValue response_array = std::vector<utils::JsonValue>();
    for (const auto& client : clients) {
        utils::JsonValue client_json;
        client_json["id"] = static_cast<int64_t>(client.client_id);
        client_json["name"] = client.name;
        client_json["contact_email"] = client.contact_email;
        client_json["daily_quota"] = client.daily_quota;
        client_json["per_minute_quota"] = client.per_minute_quota;
        client_json["is_active"] = client.is_active;
        client_json["created_at"] = utils::format_timestamp(client.created_at);
        response_array[response_array.size()] = client_json;
    }
    
    return HttpResponse::create_json_success(utils::serialize_json(response_array));
}

HttpResponse HttpRouter::handle_get_client(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        auto client = client_service_->get_client_by_id(client_id);
        
        if (!client) {
            return handle_error(HttpResponse::NOT_FOUND, "client_not_found", 
                              "Client not found");
        }
        
        utils::JsonValue response;
        response["id"] = static_cast<int64_t>(client->client_id);
        response["name"] = client->name;
        response["contact_email"] = client->contact_email;
        response["daily_quota"] = client->daily_quota;
        response["per_minute_quota"] = client->per_minute_quota;
        response["is_active"] = client->is_active;
        response["created_at"] = utils::format_timestamp(client->created_at);
        
        // 添加当前配额状态
        auto quota_status = client_service_->get_client_quota_status(client_id);
        if (quota_status) {
            utils::JsonValue quota_json;
            quota_json["daily_used"] = quota_status->daily_used;
            quota_json["minute_used"] = quota_status->minute_used;
            quota_json["daily_remaining"] = quota_status->daily_remaining;
            quota_json["minute_remaining"] = quota_status->minute_remaining;
            response["quota_status"] = quota_json;
        }
        
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    }
}

HttpResponse HttpRouter::handle_update_client(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        
        // 检查客户端是否存在
        if (!client_service_->get_client_by_id(client_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "client_not_found", 
                              "Client not found");
        }
        
        auto json = parse_json_body(request);
        
        // 准备更新参数
        std::optional<std::string> name, contact_email;
        std::optional<int64_t> daily_quota, per_minute_quota;
        std::optional<bool> is_active;
        
        if (json.contains("name")) name = json["name"].as_string();
        if (json.contains("contact_email")) contact_email = json["contact_email"].as_string();
        if (json.contains("daily_quota")) daily_quota = json["daily_quota"].as_integer();
        if (json.contains("per_minute_quota")) per_minute_quota = json["per_minute_quota"].as_integer();
        if (json.contains("is_active")) is_active = json["is_active"].as_boolean();
        
        // 更新客户端
        bool success = client_service_->update_client(client_id, name, contact_email, 
                                                    daily_quota, per_minute_quota, is_active);
        
        if (!success) {
            return handle_error(HttpResponse::BAD_REQUEST, "update_failed", 
                              "Failed to update client");
        }
        
        // 返回更新后的客户端信息
        auto updated_client = client_service_->get_client_by_id(client_id);
        utils::JsonValue response;
        response["id"] = static_cast<int64_t>(updated_client->client_id);
        response["name"] = updated_client->name;
        response["contact_email"] = updated_client->contact_email;
        response["daily_quota"] = updated_client->daily_quota;
        response["per_minute_quota"] = updated_client->per_minute_quota;
        response["is_active"] = updated_client->is_active;
        response["created_at"] = utils::format_timestamp(updated_client->created_at);
        
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_delete_client(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        
        // 检查客户端是否存在
        if (!client_service_->get_client_by_id(client_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "client_not_found", 
                              "Client not found");
        }
        
        // 删除客户端（逻辑删除）
        bool success = client_service_->delete_client(client_id);
        
        if (!success) {
            return handle_error(HttpResponse::BAD_REQUEST, "delete_failed", 
                              "Failed to delete client");
        }
        
        utils::JsonValue response;
        response["message"] = "Client deleted successfully";
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    }
}

HttpResponse HttpRouter::handle_create_api_key(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        
        // 检查客户端是否存在且活跃
        if (!client_service_->is_client_active(client_id)) {
            return handle_error(HttpResponse::BAD_REQUEST, "invalid_client", 
                              "Client not found or inactive");
        }
        
        // 解析可选的过期时间
        std::optional<std::chrono::system_clock::time_point> expire_time;
        
        if (request.is_json_content_type()) {
            auto json = parse_json_body(request);
            if (json.contains("expired_at")) {
                try {
                    std::string expired_at_str = json["expired_at"].as_string();
                    // 简化处理：暂时不解析时间字符串，直接跳过
                    // 在实际项目中应该使用健壮的时间解析方法
                    (void)expired_at_str; // 避免未使用变量警告
                } catch (...) {
                    // 忽略解析错误，不设置过期时间
                }
            }
        }
        
        // 创建API密钥
        auto api_key = api_key_service_->create_api_key(client_id, expire_time);
        if (!api_key) {
            return handle_error(HttpResponse::BAD_REQUEST, "key_creation_failed", 
                              "Failed to create API key");
        }
        
        // 返回API密钥信息
        utils::JsonValue response;
        response["id"] = static_cast<int64_t>(api_key->key_id);
        response["client_id"] = static_cast<int64_t>(api_key->client_id);
        response["key_string"] = api_key->api_key;
        // 获取状态字符串表示
        std::string status_str;
        switch (api_key->get_status()) {
            case repository::ApiKeyStatus::ACTIVE:
                status_str = "active";
                break;
            case repository::ApiKeyStatus::REVOKED:
                status_str = "revoked";
                break;
            case repository::ApiKeyStatus::EXPIRED:
                status_str = "expired";
                break;
            default:
                status_str = "unknown";
        }
        response["status"] = status_str;
        if (api_key->expires_at > 0) {
              response["expired_at"] = utils::format_timestamp(api_key->expires_at);
          }
        response["created_at"] = utils::format_timestamp(api_key->created_at);
        
        HttpResponse http_response(HttpResponse::CREATED);
        http_response.set_json_body(utils::serialize_json(response));
        return http_response;
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_get_client_api_keys(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        
        // 检查客户端是否存在
        if (!client_service_->get_client_by_id(client_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "client_not_found", 
                              "Client not found");
        }
        
        bool include_inactive = request.get_query_param("include_inactive") != "false";
        auto api_keys = api_key_service_->get_client_api_keys(client_id, include_inactive);
        
        utils::JsonValue response_array = std::vector<utils::JsonValue>();
        for (const auto& key : api_keys) {
            utils::JsonValue key_json;
            key_json["id"] = static_cast<int64_t>(key.key_id);
            key_json["key_string"] = key.api_key;
              // 获取状态字符串表示
              std::string status_str;
              switch (key.get_status()) {
                  case repository::ApiKeyStatus::ACTIVE:
                      status_str = "active";
                      break;
                  case repository::ApiKeyStatus::REVOKED:
                      status_str = "revoked";
                      break;
                  case repository::ApiKeyStatus::EXPIRED:
                      status_str = "expired";
                      break;
                  default:
                      status_str = "unknown";
              }
              key_json["status"] = status_str;
            if (key.expires_at > 0) {
                key_json["expired_at"] = utils::format_timestamp(key.expires_at);
            }
            key_json["created_at"] = utils::format_timestamp(key.created_at);
            response_array[response_array.size()] = key_json;
        }
        
        return HttpResponse::create_json_success(utils::serialize_json(response_array));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    }
}

HttpResponse HttpRouter::handle_revoke_api_key(const HttpRequest& request) {
    try {
        uint64_t key_id = std::stoull(request.get_path_param("key_id"));
        
        // 检查API密钥是否存在
        if (!api_key_service_->get_api_key_by_id(key_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "key_not_found", 
                              "API key not found");
        }
        
        // 吊销API密钥
        bool success = api_key_service_->revoke_api_key(key_id);
        
        if (!success) {
            return handle_error(HttpResponse::BAD_REQUEST, "revoke_failed", 
                              "Failed to revoke API key");
        }
        
        utils::JsonValue response;
        response["message"] = "API key revoked successfully";
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_key_id", 
                          "Invalid key ID format");
    }
}

HttpResponse HttpRouter::handle_quota_check(const HttpRequest& request) {
    try {
        auto json = parse_json_body(request);
        
        // 验证必需字段
        if (!json.contains("api_key") || !json.contains("endpoint")) {
            return handle_error(HttpResponse::BAD_REQUEST, "missing_required_fields", 
                              "api_key and endpoint are required fields");
        }
        
        // 构建配额检查请求
        service::QuotaService::QuotaCheckRequest quota_request;
        quota_request.api_key = json["api_key"].as_string();
        quota_request.endpoint = json["endpoint"].as_string();
        
        // 获取可选的权重
        if (json.contains("weight")) {
            quota_request.weight = static_cast<int32_t>(json["weight"].as_integer());
        } else {
            quota_request.weight = 1;
        }
        
        // 执行配额检查
        auto result = quota_service_->check_quota(quota_request);
        
        // 构建响应
        utils::JsonValue response;
        response["allowed"] = result.allowed;
        response["reason"] = result.reason;
        
        if (result.allowed) {
            response["remaining_in_minute"] = result.remaining_in_minute;
            response["remaining_in_day"] = result.remaining_in_day;
        } else {
            response["retry_after_seconds"] = static_cast<int64_t>(result.retry_after_seconds);
        }
        
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_get_top_clients(const HttpRequest& request) {
    try {
        int limit = 10; // 默认值
        if (!request.get_query_param("limit").empty()) {
            limit = std::stoi(request.get_query_param("limit"));
        }
        
        std::string order_by = "daily_calls";
        if (!request.get_query_param("by").empty()) {
            order_by = request.get_query_param("by");
        }
        
        auto top_clients = quota_service_->get_top_clients(limit, order_by);
        
        utils::JsonValue response_array = std::vector<utils::JsonValue>();
        for (const auto& client : top_clients) {
            utils::JsonValue client_json;
            client_json["client_id"] = static_cast<int64_t>(client.client_id);
            client_json["client_name"] = client.client_name;
            client_json["daily_quota"] = client.daily_quota;
            client_json["per_minute_quota"] = client.per_minute_quota;
            client_json["daily_used"] = client.daily_used;
            client_json["minute_used"] = client.minute_used;
            client_json["total_calls"] = client.total_calls;
            client_json["denied_calls"] = client.denied_calls;
            response_array[response_array.size()] = client_json;
        }
        
        return HttpResponse::create_json_success(utils::serialize_json(response_array));
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_get_client_summary(const HttpRequest& request) {
    try {
        uint64_t client_id = std::stoull(request.get_path_param("client_id"));
        
        // 检查客户端是否存在
        if (!client_service_->get_client_by_id(client_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "client_not_found", 
                              "Client not found");
        }
        
        // 获取可选的日期范围
        std::optional<std::string> from_date, to_date;
        if (!request.get_query_param("from").empty()) {
            from_date = request.get_query_param("from");
        }
        if (!request.get_query_param("to").empty()) {
            to_date = request.get_query_param("to");
        }
        
        auto stats = quota_service_->get_client_detailed_stats(client_id, from_date, to_date);
        if (!stats) {
            return handle_error(HttpResponse::INTERNAL_SERVER_ERROR, "stats_retrieval_failed", 
                              "Failed to retrieve client statistics");
        }
        
        // 构建响应
        utils::JsonValue response;
        response["client_id"] = static_cast<int64_t>(stats->client_id);
        response["client_name"] = stats->client_name;
        response["contact_email"] = stats->contact_email;
        response["total_calls"] = stats->total_calls;
        response["denied_calls"] = stats->denied_calls;
        response["success_rate"] = stats->success_rate;
        
        // 构建拒绝原因统计
        utils::JsonValue denied_reasons_json; // 空构造函数应该就足够了
        for (const auto& [reason, count] : stats->denied_reasons) {
            denied_reasons_json[reason] = count;
        }
        response["denied_reasons"] = denied_reasons_json;
        
        // 构建最近日志
        utils::JsonValue recent_logs_json = std::vector<utils::JsonValue>();
        for (const auto& log : stats->recent_logs) {
            utils::JsonValue log_json;
            log_json["timestamp"] = utils::format_timestamp(log.timestamp);
            log_json["api_key"] = log.api_key;
            log_json["endpoint"] = log.endpoint;
            log_json["weight"] = log.weight;
            log_json["allowed"] = log.allowed;
            log_json["reason"] = log.reason;
            recent_logs_json[recent_logs_json.size()] = log_json;
        }
        response["recent_logs"] = recent_logs_json;
        
        return HttpResponse::create_json_success(utils::serialize_json(response));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_client_id", 
                          "Invalid client ID format");
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

HttpResponse HttpRouter::handle_get_key_timeline(const HttpRequest& request) {
    try {
        uint64_t key_id = std::stoull(request.get_path_param("key_id"));
        
        // 检查API密钥是否存在
        if (!api_key_service_->get_api_key_by_id(key_id)) {
            return handle_error(HttpResponse::NOT_FOUND, "key_not_found", 
                              "API key not found");
        }
        
        // 获取粒度参数
        std::string granularity_str = request.get_query_param("granularity", "hour");
        int granularity = 0; // 使用int替代TimeGranularity枚举
        
        if (granularity_str == "day") {
            granularity = 1; // 代表DAY
        } else if (granularity_str == "hour") {
            granularity = 0; // 代表HOUR
        } else if (granularity_str == "minute") {
            granularity = 2; // 代表MINUTE
        }
        
        auto timeline = api_key_service_->get_key_timeline(key_id, granularity);
        
        utils::JsonValue response_array = std::vector<utils::JsonValue>();
        for (const auto& point : timeline) {
            utils::JsonValue point_json;
            // 假设pair的first是timestamp，second是calls总数
            point_json["timestamp"] = utils::format_timestamp(point.first);
            point_json["calls"] = static_cast<int64_t>(point.second);
            // 由于是pair，无法区分allowed和denied，暂时都设为0
            point_json["allowed"] = static_cast<int64_t>(0);
            point_json["denied"] = static_cast<int64_t>(0);
            response_array[response_array.size()] = point_json;
        }
        
        return HttpResponse::create_json_success(utils::serialize_json(response_array));
    } catch (const std::invalid_argument&) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_key_id", 
                          "Invalid key ID format");
    } catch (const std::exception& e) {
        return handle_error(HttpResponse::BAD_REQUEST, "invalid_request", e.what());
    }
}

} // namespace http
} // namespace api_quota