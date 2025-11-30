#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include "models.h"
#include "product_service.h"
#include <string>
#include <optional>
#include <vector>
#include <memory>

// 订单服务接口
class OrderService {
public:
    virtual ~OrderService() = default;
    
    // 创建订单
    virtual std::optional<Order> createOrder(const OrderCreateRequest& request) = 0;
    
    // 获取订单详情
    virtual std::optional<Order> getOrderById(int id) = 0;
    
    // 获取订单列表（分页）
    virtual OrderListResponse getOrders(const Pagination& pagination, 
                                       const std::optional<std::string>& status = std::nullopt,
                                       const std::optional<std::string>& startDate = std::nullopt,
                                       const std::optional<std::string>& endDate = std::nullopt) = 0;
    
    // 更新订单状态
    virtual bool updateOrderStatus(int orderId, const OrderStatusUpdateRequest& request) = 0;
    
    // 获取订单的订单项
    virtual std::vector<OrderItem> getOrderItems(int orderId) = 0;
};

// 订单服务实现类
class OrderServiceImpl : public OrderService {
public:
    OrderServiceImpl(std::shared_ptr<ProductService> productService);
    ~OrderServiceImpl() = default;
    
    std::optional<Order> createOrder(const OrderCreateRequest& request) override;
    std::optional<Order> getOrderById(int id) override;
    OrderListResponse getOrders(const Pagination& pagination, 
                               const std::optional<std::string>& status = std::nullopt,
                               const std::optional<std::string>& startDate = std::nullopt,
                               const std::optional<std::string>& endDate = std::nullopt) override;
    bool updateOrderStatus(int orderId, const OrderStatusUpdateRequest& request) override;
    std::vector<OrderItem> getOrderItems(int orderId) override;
    
private:
    std::shared_ptr<ProductService> productService_;
    
    // 验证订单创建请求
    bool validateOrderCreateRequest(const OrderCreateRequest& request);
    
    // 验证订单状态更新
    bool validateStatusTransition(OrderStatus currentStatus, OrderStatus newStatus);
    
    // 构建订单对象
    Order buildOrder(const std::map<std::string, std::string>& data);
    
    // 构建订单项对象
    OrderItem buildOrderItem(const std::map<std::string, std::string>& data);
    
    // 回滚订单库存
    bool restockOrderInventory(int orderId);
};

#endif // ORDER_SERVICE_H
