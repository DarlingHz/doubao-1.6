#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <optional>
#include <chrono>

// 商品状态枚举
enum class OrderStatus {
    PENDING,
    PAID,
    CANCELLED,
    SHIPPED
};

// 订单状态转换为字符串
std::string orderStatusToString(OrderStatus status);

// 字符串转换为订单状态
std::optional<OrderStatus> stringToOrderStatus(const std::string& statusStr);

// 商品模型
struct Product {
    int id;                    // 商品ID
    std::string name;          // 商品名称
    std::string sku;           // 商品SKU
    double price;              // 商品价格
    int stock;                 // 库存数量
    int reorder_threshold;     // 重新订购阈值
    std::string created_at;    // 创建时间
    std::string updated_at;    // 更新时间
    
    Product() : id(0), price(0.0), stock(0), reorder_threshold(0) {}
    
    // 检查库存是否低于阈值
    bool isLowStock() const {
        return stock <= reorder_threshold;
    }
    
    // 检查库存是否充足
    bool hasEnoughStock(int quantity) const {
        return stock >= quantity;
    }
};

// 订单商品项模型
struct OrderItem {
    int id;             // 订单项ID
    int order_id;       // 订单ID
    int product_id;     // 商品ID
    int quantity;       // 数量
    double unit_price;  // 单价
    double subtotal;    // 小计金额
    
    // 计算小计金额
    void calculateSubtotal() {
        subtotal = unit_price * quantity;
    }
};

// 订单模型
struct Order {
    int id;                     // 订单ID
    OrderStatus status;         // 订单状态
    double total_amount;        // 订单总金额
    std::string created_at;     // 创建时间
    std::string updated_at;     // 更新时间
    std::vector<OrderItem> items; // 订单商品列表
    
    Order() : id(0), total_amount(0.0), status(OrderStatus::PENDING) {}
    
    // 计算订单总金额
    void calculateTotalAmount() {
        total_amount = 0.0;
        for (auto& item : items) {
            item.calculateSubtotal();
            total_amount += item.subtotal;
        }
    }
    
    // 检查订单是否可取消
    bool canBeCancelled() const {
        return status == OrderStatus::PENDING || status == OrderStatus::PAID;
    }
};

// 库存变动日志模型
struct InventoryLog {
    int id;             // 日志ID
    int product_id;     // 商品ID
    int delta;          // 变动数量（正数为增加，负数为减少）
    std::string reason; // 变动原因
    int previous_stock; // 变动前库存
    int current_stock;  // 变动后库存
    std::string created_at; // 创建时间
    
    InventoryLog() : id(0), product_id(0), delta(0), previous_stock(0), current_stock(0) {}
};

// 商品创建请求
struct ProductCreateRequest {
    std::string name;
    std::string sku;
    double price;
    int initial_stock;
    int reorder_threshold;
};

// 商品更新请求
struct ProductUpdateRequest {
    std::optional<std::string> name;
    std::optional<double> price;
    std::optional<int> reorder_threshold;
};

// 库存调整请求
struct StockAdjustRequest {
    int delta;
    std::string reason;
};

// 订单创建请求项
struct OrderCreateItem {
    int product_id;
    int quantity;
};

// 订单创建请求
struct OrderCreateRequest {
    std::vector<OrderCreateItem> items;
};

// 订单状态更新请求
struct OrderStatusUpdateRequest {
    std::string status;
    bool restock = false;
};

// 分页参数
struct Pagination {
    int page;
    int page_size;
    std::string keyword;  // 搜索关键词
    
    Pagination() : page(1), page_size(10) {}
    
    // 获取偏移量
    int getOffset() const {
        return (page - 1) * page_size;
    }
};

// 分页响应
struct PaginationResponse {
    int total;
    int page;
    int page_size;
    int total_pages;
    
    PaginationResponse() : total(0), page(1), page_size(10), total_pages(0) {}
    
    void calculateTotalPages() {
        total_pages = (total + page_size - 1) / page_size;
    }
};

// 商品分页响应
struct ProductListResponse {
    PaginationResponse pagination;
    std::vector<Product> products;
};

// 订单分页响应
struct OrderListResponse {
    PaginationResponse pagination;
    std::vector<Order> orders;
};

// 每日统计响应
struct DailySummaryResponse {
    std::string date;
    int total_orders;
    int paid_orders;
    int shipped_orders;
    int cancelled_orders;
    double total_sales_amount;
};

#endif // MODELS_H
