#include "stats_service.h"
#include "database.h"
#include <iostream>
#include <sstream>
#include <regex>

StatsServiceImpl::StatsServiceImpl(std::shared_ptr<ProductService> productService, 
                                 std::shared_ptr<OrderService> orderService)
    : productService_(std::move(productService)), 
      orderService_(std::move(orderService)) {
}

std::vector<Product> StatsServiceImpl::getLowStockProducts() {
    // 直接复用商品服务的低库存查询功能
    return productService_->getLowStockProducts();
}

std::optional<DailySummaryResponse> StatsServiceImpl::getDailySummary(const std::string& date) {
    // 验证日期格式
    if (!validateDateFormat(date)) {
        return std::nullopt;
    }
    
    auto& db = Database::getInstance();
    DailySummaryResponse summary;
    summary.date = date;
    
    // 构建日期范围（当天的开始和结束）
    std::string startDate = date + " 00:00:00";
    std::string endDate = date + " 23:59:59";
    
    // 获取订单总数
    std::stringstream totalOrdersSql;
    totalOrdersSql << "SELECT COUNT(*) as count FROM orders WHERE created_at >= '" 
                   << startDate << "' AND created_at <= '" << endDate << "'";
    
    auto totalOrdersResult = db.query(totalOrdersSql.str());
    if (!totalOrdersResult.empty()) {
        summary.total_orders = std::stoi(totalOrdersResult[0]["count"]);
    }
    
    // 获取已支付订单数量
    std::stringstream paidOrdersSql;
    paidOrdersSql << "SELECT COUNT(*) as count FROM orders WHERE status = 'PAID' AND created_at >= '" 
                  << startDate << "' AND created_at <= '" << endDate << "'";
    
    auto paidOrdersResult = db.query(paidOrdersSql.str());
    if (!paidOrdersResult.empty()) {
        summary.paid_orders = std::stoi(paidOrdersResult[0]["count"]);
    }
    
    // 获取已发货订单数量
    std::stringstream shippedOrdersSql;
    shippedOrdersSql << "SELECT COUNT(*) as count FROM orders WHERE status = 'SHIPPED' AND created_at >= '" 
                     << startDate << "' AND created_at <= '" << endDate << "'";
    
    auto shippedOrdersResult = db.query(shippedOrdersSql.str());
    if (!shippedOrdersResult.empty()) {
        summary.shipped_orders = std::stoi(shippedOrdersResult[0]["count"]);
    }
    
    // 获取已取消订单数量
    std::stringstream cancelledOrdersSql;
    cancelledOrdersSql << "SELECT COUNT(*) as count FROM orders WHERE status = 'CANCELLED' AND created_at >= '" 
                       << startDate << "' AND created_at <= '" << endDate << "'";
    
    auto cancelledOrdersResult = db.query(cancelledOrdersSql.str());
    if (!cancelledOrdersResult.empty()) {
        summary.cancelled_orders = std::stoi(cancelledOrdersResult[0]["count"]);
    }
    
    // 获取总销售额（已支付和已发货的订单）
    std::stringstream totalSalesSql;
    totalSalesSql << "SELECT SUM(total_amount) as total FROM orders WHERE "
                  << "(status = 'PAID' OR status = 'SHIPPED') AND "
                  << "created_at >= '" << startDate << "' AND created_at <= '" << endDate << "'";
    
    auto totalSalesResult = db.query(totalSalesSql.str());
    if (!totalSalesResult.empty() && !totalSalesResult[0]["total"].empty()) {
        summary.total_sales_amount = std::stod(totalSalesResult[0]["total"]);
    }
    
    return summary;
}

bool StatsServiceImpl::validateDateFormat(const std::string& date) {
    // 验证日期格式 YYYY-MM-DD
    std::regex datePattern("^(\\d{4})-(\\d{2})-(\\d{2})$");
    std::smatch match;
    
    if (!std::regex_match(date, match, datePattern)) {
        return false;
    }
    
    // 进一步验证日期的有效性（简单验证，实际项目中可以更严格）
    int year = std::stoi(match[1]);
    int month = std::stoi(match[2]);
    int day = std::stoi(match[3]);
    
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    
    // 简单检查闰年和每月天数
    if (month == 2) {
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (day > (isLeapYear ? 29 : 28)) return false;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        if (day > 30) return false;
    }
    
    return true;
}
