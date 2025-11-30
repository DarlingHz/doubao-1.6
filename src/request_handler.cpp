#include "request_handler.h"
#include "product_service.h"
#include "order_service.h"
#include "stats_service.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <map>
#include <memory>

RequestHandler::RequestHandler() {
    // 初始化服务实例
    productService_ = std::make_shared<ProductServiceImpl>();
    orderService_ = std::make_shared<OrderServiceImpl>(productService_);
    statsService_ = std::make_shared<StatsServiceImpl>(productService_, orderService_);
}

std::string RequestHandler::processRequest(const std::string& request) {
    // 解析请求行
    auto requestLineEnd = request.find("\r\n");
    if (requestLineEnd == std::string::npos) {
        return createErrorResponse(400, "Invalid request format");
    }
    
    std::string requestLine = request.substr(0, requestLineEnd);
    std::stringstream ss(requestLine);
    std::string method, path, version;
    ss >> method >> path >> version;
    
    // 解析路径和参数
    std::string pathWithoutQuery;
    std::map<std::string, std::string> queryParams;
    parsePathAndQuery(path, pathWithoutQuery, queryParams);
    
    // 解析请求体（如果有）
    std::string requestBody;
    auto bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        requestBody = request.substr(bodyStart + 4);
    }
    
    // 路由请求
    if (method == "POST" && pathWithoutQuery == "/products") {
        return handleCreateProduct(requestBody);
    } else if (method == "GET" && pathWithoutQuery == "/products") {
        return handleGetProducts(queryParams);
    } else if (method == "GET" && std::regex_match(pathWithoutQuery, std::regex("^/products/\\d+$"))) {
        int productId = extractIdFromPath(pathWithoutQuery);
        return handleGetProductById(productId);
    } else if (method == "PUT" && std::regex_match(pathWithoutQuery, std::regex("^/products/\\d+$"))) {
        int productId = extractIdFromPath(pathWithoutQuery);
        return handleUpdateProduct(productId, requestBody);
    } else if (method == "POST" && std::regex_match(pathWithoutQuery, std::regex("^/products/\\d+/adjust_stock$"))) {
        int productId = extractIdFromPath(pathWithoutQuery, "/adjust_stock");
        return handleAdjustStock(productId, requestBody);
    } else if (method == "POST" && pathWithoutQuery == "/orders") {
        return handleCreateOrder(requestBody);
    } else if (method == "GET" && pathWithoutQuery == "/orders") {
        return handleGetOrders(queryParams);
    } else if (method == "GET" && std::regex_match(pathWithoutQuery, std::regex("^/orders/\\d+$"))) {
        int orderId = extractIdFromPath(pathWithoutQuery);
        return handleGetOrderById(orderId);
    } else if (method == "POST" && std::regex_match(pathWithoutQuery, std::regex("^/orders/\\d+/status$"))) {
        int orderId = extractIdFromPath(pathWithoutQuery, "/status");
        return handleUpdateOrderStatus(orderId, requestBody);
    } else if (method == "GET" && pathWithoutQuery == "/stats/low_stock") {
        return handleGetLowStockProducts();
    } else if (method == "GET" && pathWithoutQuery == "/stats/daily_summary") {
        return handleGetDailySummary(queryParams);
    }
    
    // 404 未找到
    return createErrorResponse(404, "Not found");
}

std::string RequestHandler::handleCreateProduct(const std::string& requestBody) {
    try {
        auto json = ::parseJson(requestBody);
        
        ProductCreateRequest request;
        request.name = json["name"];
        request.sku = json["sku"];
        request.price = std::stod(json["price"]);
        request.initial_stock = std::stoi(json["initial_stock"]);
        
        if (json.find("reorder_threshold") != json.end()) {
            request.reorder_threshold = std::stoi(json["reorder_threshold"]);
        }
        
        auto product = productService_->createProduct(request);
        
        if (product) {
            return createJsonResponse(201, ::productToJson(*product));
        } else {
            return createErrorResponse(400, "Failed to create product");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling create product: " << e.what() << std::endl;
        return createErrorResponse(400, "Invalid request data");
    }
}

std::string RequestHandler::handleGetProducts(const std::map<std::string, std::string>& queryParams) {
    Pagination pagination;
    
    if (queryParams.find("page") != queryParams.end()) {
        pagination.page = std::stoi(queryParams.at("page"));
    }
    
    if (queryParams.find("page_size") != queryParams.end()) {
        pagination.page_size = std::stoi(queryParams.at("page_size"));
    }
    
    if (queryParams.find("keyword") != queryParams.end()) {
        pagination.keyword = queryParams.at("keyword");
    }
    
    auto response = productService_->getProducts(pagination);
    
    std::stringstream json;
    json << "{";
    json << "\"pagination\":{";
    json << "\"total\":" << response.pagination.total << ",";
    json << "\"page\":" << response.pagination.page << ",";
    json << "\"page_size\":" << response.pagination.page_size << ",";
    json << "\"total_pages\":" << response.pagination.total_pages;
    json << "},";
    json << "\"products\":[";
    
    bool first = true;
    for (const auto& product : response.products) {
        if (!first) json << ",";
        json << ::productToJson(product);
        first = false;
    }
    
    json << "]";
    json << "}";
    
    return createJsonResponse(200, json.str());
}

std::string RequestHandler::handleGetProductById(int productId) {
    auto product = productService_->getProductById(productId);
    
    if (product) {
        return createJsonResponse(200, ::productToJson(*product));
    } else {
        return createErrorResponse(404, "Product not found");
    }
}

std::string RequestHandler::handleUpdateProduct(int productId, const std::string& requestBody) {
    try {
        auto json = ::parseJson(requestBody);
        
        ProductUpdateRequest request;
        
        if (json.find("name") != json.end()) {
            request.name = json["name"];
        }
        
        if (json.find("price") != json.end()) {
            request.price = std::stod(json["price"]);
        }
        
        if (json.find("reorder_threshold") != json.end()) {
            request.reorder_threshold = std::stoi(json["reorder_threshold"]);
        }
        
        auto product = productService_->updateProduct(productId, request);
        
        if (product) {
            return createJsonResponse(200, ::productToJson(*product));
        } else {
            return createErrorResponse(404, "Product not found or update failed");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling update product: " << e.what() << std::endl;
        return createErrorResponse(400, "Invalid request data");
    }
}

std::string RequestHandler::handleAdjustStock(int productId, const std::string& requestBody) {
    try {
        auto json = ::parseJson(requestBody);
        int delta = std::stoi(json["delta"]);
        std::string reason = json["reason"];
        
        if (productService_->adjustStock(productId, delta, reason)) {
            return createJsonResponse(200, "{\"status\":\"success\"}");
        } else {
            return createErrorResponse(400, "Failed to adjust stock");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling adjust stock: " << e.what() << std::endl;
        return createErrorResponse(400, "Invalid request data");
    }
}

std::string RequestHandler::handleCreateOrder(const std::string& requestBody) {
    try {
        // 直接从请求体中提取 items 数组内容
        size_t itemsStart = requestBody.find("items");
        if (itemsStart != std::string::npos) {
            itemsStart = requestBody.find('[', itemsStart);
            if (itemsStart != std::string::npos) {
                size_t itemsEnd = requestBody.rfind(']');
                if (itemsEnd != std::string::npos) {
                    std::string itemsArrayStr = requestBody.substr(itemsStart, itemsEnd - itemsStart + 1);
                    auto itemsArray = ::parseJsonArray(itemsArrayStr);
                    
                    OrderCreateRequest request;
                    for (const auto& item : itemsArray) {
                        OrderCreateItem orderItem;
                        orderItem.product_id = std::stoi(item.at("product_id"));
                        orderItem.quantity = std::stoi(item.at("quantity"));
                        request.items.push_back(orderItem);
                    }
                    
                    auto order = orderService_->createOrder(request);
                    
                    if (order) {
                        return createJsonResponse(201, ::orderToJson(*order));
                    } else {
                        return createErrorResponse(400, "Failed to create order");
                    }
                }
            }
        }
        
        return createErrorResponse(400, "Invalid order items format");
    } catch (const std::exception& e) {
        std::cerr << "Error handling create order: " << e.what() << std::endl;
        return createErrorResponse(400, "Invalid request data");
    }
}

std::string RequestHandler::handleGetOrders(const std::map<std::string, std::string>& queryParams) {
    Pagination pagination;
    std::optional<std::string> status, startDate, endDate;
    
    if (queryParams.find("page") != queryParams.end()) {
        pagination.page = std::stoi(queryParams.at("page"));
    }
    
    if (queryParams.find("page_size") != queryParams.end()) {
        pagination.page_size = std::stoi(queryParams.at("page_size"));
    }
    
    if (queryParams.find("status") != queryParams.end()) {
        status = queryParams.at("status");
    }
    
    if (queryParams.find("start_date") != queryParams.end()) {
        startDate = queryParams.at("start_date");
    }
    
    if (queryParams.find("end_date") != queryParams.end()) {
        endDate = queryParams.at("end_date");
    }
    
    auto response = orderService_->getOrders(pagination, status, startDate, endDate);
    
    std::stringstream json;
    json << "{";
    json << "\"pagination\":{";
    json << "\"total\":" << response.pagination.total << ",";
    json << "\"page\":" << response.pagination.page << ",";
    json << "\"page_size\":" << response.pagination.page_size << ",";
    json << "\"total_pages\":" << response.pagination.total_pages;
    json << "},";
    json << "\"orders\":[";
    
    bool first = true;
    for (const auto& order : response.orders) {
        if (!first) json << ",";
        json << ::orderToJson(order);
        first = false;
    }
    
    json << "]";
    json << "}";
    
    return createJsonResponse(200, json.str());
}

std::string RequestHandler::handleGetOrderById(int orderId) {
    auto order = orderService_->getOrderById(orderId);
    
    if (order) {
        return createJsonResponse(200, ::orderToJson(*order));
    } else {
        return createErrorResponse(404, "Order not found");
    }
}

std::string RequestHandler::handleUpdateOrderStatus(int orderId, const std::string& requestBody) {
    try {
        auto json = ::parseJson(requestBody);
        OrderStatusUpdateRequest request;
        request.status = json["status"];
        
        if (json.find("restock") != json.end()) {
            request.restock = (json["restock"] == "true");
        }
        
        if (orderService_->updateOrderStatus(orderId, request)) {
            return createJsonResponse(200, "{\"status\":\"success\"}");
        } else {
            return createErrorResponse(400, "Failed to update order status");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling update order status: " << e.what() << std::endl;
        return createErrorResponse(400, "Invalid request data");
    }
}

std::string RequestHandler::handleGetLowStockProducts() {
    auto products = statsService_->getLowStockProducts();
    
    std::stringstream json;
    json << "[";
    
    bool first = true;
    for (const auto& product : products) {
        if (!first) json << ",";
        json << ::productToJson(product);
        first = false;
    }
    
    json << "]";
    
    return createJsonResponse(200, json.str());
}

std::string RequestHandler::handleGetDailySummary(const std::map<std::string, std::string>& queryParams) {
    if (queryParams.find("date") == queryParams.end()) {
        return createErrorResponse(400, "Date parameter is required");
    }
    
    auto summary = statsService_->getDailySummary(queryParams.at("date"));
    
    if (summary) {
        std::stringstream json;
        json << "{";
        json << "\"date\":\"" << summary->date << "\",";
        json << "\"total_orders\":" << summary->total_orders << ",";
        json << "\"paid_orders\":" << summary->paid_orders << ",";
        json << "\"shipped_orders\":" << summary->shipped_orders << ",";
        json << "\"cancelled_orders\":" << summary->cancelled_orders << ",";
        json << "\"total_sales_amount\":" << summary->total_sales_amount;
        json << "}";
        
        return createJsonResponse(200, json.str());
    } else {
        return createErrorResponse(400, "Invalid date format");
    }
}

void RequestHandler::parsePathAndQuery(const std::string& path, 
                                      std::string& pathWithoutQuery, 
                                      std::map<std::string, std::string>& queryParams) {
    auto queryStart = path.find('?');
    
    if (queryStart != std::string::npos) {
        pathWithoutQuery = path.substr(0, queryStart);
        
        // 解析查询参数
        std::string queryString = path.substr(queryStart + 1);
        size_t pos = 0;
        
        while (pos < queryString.size()) {
            size_t ampPos = queryString.find('&', pos);
            std::string param;
            
            if (ampPos != std::string::npos) {
                param = queryString.substr(pos, ampPos - pos);
                pos = ampPos + 1;
            } else {
                param = queryString.substr(pos);
                pos = queryString.size();
            }
            
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                queryParams[key] = value;
            }
        }
    } else {
        pathWithoutQuery = path;
    }
}

int RequestHandler::extractIdFromPath(const std::string& path, const std::string& suffix) {
    std::string pathToProcess = path;
    
    if (!suffix.empty() && pathToProcess.size() > suffix.size() && 
        pathToProcess.substr(pathToProcess.size() - suffix.size()) == suffix) {
        pathToProcess = pathToProcess.substr(0, pathToProcess.size() - suffix.size());
    }
    
    auto lastSlash = pathToProcess.rfind('/');
    if (lastSlash != std::string::npos && lastSlash < pathToProcess.size() - 1) {
        try {
            return std::stoi(pathToProcess.substr(lastSlash + 1));
        } catch (...) {
            return -1;
        }
    }
    
    return -1;
}

std::string RequestHandler::createJsonResponse(int statusCode, const std::string& body) {
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " OK\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

std::string RequestHandler::createErrorResponse(int statusCode, const std::string& message) {
    std::stringstream body;
    body << "{\"error\":\"" << message << "\"}";
    
    return createJsonResponse(statusCode, body.str());
}
