#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "models.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

// 前置声明
class ProductService;
class OrderService;
class StatsService;

// HTTP 请求处理器
class RequestHandler {
public:
    RequestHandler();
    ~RequestHandler() = default;
    
    // 处理 HTTP 请求
    std::string processRequest(const std::string& request);
    
private:
    std::shared_ptr<ProductService> productService_;
    std::shared_ptr<OrderService> orderService_;
    std::shared_ptr<StatsService> statsService_;
    
    // 路由处理函数
    std::string handleCreateProduct(const std::string& requestBody);
    std::string handleGetProducts(const std::map<std::string, std::string>& queryParams);
    std::string handleGetProductById(int productId);
    std::string handleUpdateProduct(int productId, const std::string& requestBody);
    std::string handleAdjustStock(int productId, const std::string& requestBody);
    
    std::string handleCreateOrder(const std::string& requestBody);
    std::string handleGetOrders(const std::map<std::string, std::string>& queryParams);
    std::string handleGetOrderById(int orderId);
    std::string handleUpdateOrderStatus(int orderId, const std::string& requestBody);
    
    std::string handleGetLowStockProducts();
    std::string handleGetDailySummary(const std::map<std::string, std::string>& queryParams);
    
    // 辅助函数
    void parsePathAndQuery(const std::string& path, 
                          std::string& pathWithoutQuery, 
                          std::map<std::string, std::string>& queryParams);
    
    int extractIdFromPath(const std::string& path, const std::string& suffix = "");
    
    std::string createJsonResponse(int statusCode, const std::string& body);
    std::string createErrorResponse(int statusCode, const std::string& message);
    
    // JSON 解析辅助函数
    std::map<std::string, std::string> parseJson(const std::string& json);
    std::string productToJson(const Product& product);
    std::string orderToJson(const Order& order);
};

#endif // REQUEST_HANDLER_H
