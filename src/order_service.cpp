#include "order_service.h"
#include "database.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

OrderServiceImpl::OrderServiceImpl(std::shared_ptr<ProductService> productService) 
    : productService_(std::move(productService)) {
}

std::optional<Order> OrderServiceImpl::createOrder(const OrderCreateRequest& request) {
    // 验证请求
    if (!validateOrderCreateRequest(request)) {
        return std::nullopt;
    }
    
    auto& db = Database::getInstance();
    
    // 开始事务 - 确保订单创建和库存扣减的原子性
    if (!db.beginTransaction()) {
        std::cerr << "Failed to begin transaction: " << db.getErrorMessage() << std::endl;
        return std::nullopt;
    }
    
    try {
        // 批量获取所有商品信息 - 优化性能，避免多次单条查询
        std::vector<int> productIds;
        for (const auto& item : request.items) {
            productIds.push_back(item.product_id);
        }
        
        auto products = productService_->getProductsByIds(productIds);
        
        // 构建商品ID到商品对象的映射 - 便于快速查找
        std::unordered_map<int, Product> productMap;
        for (const auto& product : products) {
            productMap[product.id] = product;
        }
        
        // 检查所有商品库存是否充足
        for (const auto& item : request.items) {
            auto it = productMap.find(item.product_id);
            if (it == productMap.end() || !it->second.hasEnoughStock(item.quantity)) {
                db.rollbackTransaction();
                return std::nullopt;
            }
        }
        
        // 创建订单
        Order order;
        
        // 构建订单项并计算总金额
        for (const auto& item : request.items) {
            const auto& product = productMap[item.product_id];
            
            OrderItem orderItem;
            orderItem.product_id = product.id;
            orderItem.quantity = item.quantity;
            orderItem.unit_price = product.price;
            orderItem.calculateSubtotal();
            
            order.items.push_back(orderItem);
        }
        
        // 计算订单总金额
        order.calculateTotalAmount();
        
        // 插入订单主表
        std::string insertOrderSql = "INSERT INTO orders (status, total_amount) VALUES (?, ?);";
        PreparedStatement orderStmt(db.getConnection(), insertOrderSql);
        orderStmt.bind(1, orderStatusToString(order.status));
        orderStmt.bind(2, order.total_amount);
        
        if (!orderStmt.execute()) {
            std::cerr << "Failed to insert order: " << orderStmt.getErrorMessage() << std::endl;
            db.rollbackTransaction();
            return std::nullopt;
        }
        
        // 获取订单ID
        int64_t orderId = db.getLastInsertId();
        order.id = static_cast<int>(orderId);
        
        // 插入订单项
        std::string insertItemSql = "INSERT INTO order_items (order_id, product_id, quantity, unit_price, subtotal) VALUES (?, ?, ?, ?, ?);";
        
        for (auto& item : order.items) {
            PreparedStatement itemStmt(db.getConnection(), insertItemSql);
            itemStmt.bind(1, static_cast<int>(orderId));
            itemStmt.bind(2, item.product_id);
            itemStmt.bind(3, item.quantity);
            itemStmt.bind(4, item.unit_price);
            itemStmt.bind(5, item.subtotal);
            
            if (!itemStmt.execute()) {
                std::cerr << "Failed to insert order item: " << itemStmt.getErrorMessage() << std::endl;
                db.rollbackTransaction();
                return std::nullopt;
            }
            
            // 更新订单项ID
            item.id = static_cast<int>(db.getLastInsertId());
            item.order_id = static_cast<int>(orderId);
            
            // 扣减库存
            if (!productService_->adjustStock(item.product_id, -item.quantity, "order_create")) {
                std::cerr << "Failed to adjust stock for product " << item.product_id << std::endl;
                db.rollbackTransaction();
                return std::nullopt;
            }
        }
        
        // 提交事务
        if (!db.commitTransaction()) {
            std::cerr << "Failed to commit transaction: " << db.getErrorMessage() << std::endl;
            return std::nullopt;
        }
        
        // 重新查询订单信息（包含创建时间等）
        return getOrderById(order.id);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in createOrder: " << e.what() << std::endl;
        db.rollbackTransaction();
        return std::nullopt;
    }
}

std::optional<Order> OrderServiceImpl::getOrderById(int id) {
    auto& db = Database::getInstance();
    std::stringstream ss;
    ss << "SELECT * FROM orders WHERE id = " << id;
    
    auto results = db.query(ss.str());
    if (results.empty()) {
        return std::nullopt;
    }
    
    Order order = buildOrder(results[0]);
    
    // 获取订单项
    order.items = getOrderItems(id);
    
    return order;
}

OrderListResponse OrderServiceImpl::getOrders(const Pagination& pagination, 
                                             const std::optional<std::string>& status,
                                             const std::optional<std::string>& startDate,
                                             const std::optional<std::string>& endDate) {
    OrderListResponse response;
    auto& db = Database::getInstance();
    
    // 构建查询条件
    std::stringstream whereClause;
    whereClause << " WHERE 1=1";
    
    if (status.has_value()) {
        whereClause << " AND status = '" << status.value() << "'";
    }
    
    if (startDate.has_value()) {
        whereClause << " AND created_at >= '" << startDate.value() << "'";
    }
    
    if (endDate.has_value()) {
        whereClause << " AND created_at <= '" << endDate.value() << "'";
    }
    
    // 获取总数
    std::stringstream countSql;
    countSql << "SELECT COUNT(*) as count FROM orders" << whereClause.str();
    
    auto countResult = db.query(countSql.str());
    if (!countResult.empty()) {
        response.pagination.total = std::stoi(countResult[0]["count"]);
    }
    
    // 构建分页查询
    std::stringstream querySql;
    querySql << "SELECT * FROM orders" << whereClause.str()
             << " ORDER BY created_at DESC LIMIT " << pagination.page_size
             << " OFFSET " << pagination.getOffset();
    
    auto orderResults = db.query(querySql.str());
    
    // 构建订单列表
    for (const auto& row : orderResults) {
        Order order = buildOrder(row);
        // 为了性能考虑，列表查询不加载订单项详情
        response.orders.push_back(order);
    }
    
    // 设置分页信息
    response.pagination.page = pagination.page;
    response.pagination.page_size = pagination.page_size;
    response.pagination.calculateTotalPages();
    
    return response;
}

bool OrderServiceImpl::updateOrderStatus(int orderId, const OrderStatusUpdateRequest& request) {
    auto& db = Database::getInstance();
    
    // 获取当前订单
    auto order = getOrderById(orderId);
    if (!order) {
        return false;
    }
    
    // 转换状态字符串
    auto newStatus = stringToOrderStatus(request.status);
    if (!newStatus) {
        return false;
    }
    
    // 验证状态转换是否有效
    if (!validateStatusTransition(order->status, newStatus.value())) {
        return false;
    }
    
    // 开始事务
    if (!db.beginTransaction()) {
        return false;
    }
    
    try {
        // 更新订单状态
        std::stringstream updateSql;
        updateSql << "UPDATE orders SET status = '" << request.status 
                  << "', updated_at = CURRENT_TIMESTAMP WHERE id = " << orderId;
        
        if (!db.execute(updateSql.str())) {
            std::cerr << "Failed to update order status: " << db.getErrorMessage() << std::endl;
            db.rollbackTransaction();
            return false;
        }
        
        // 如果订单被取消且需要回滚库存
        if (newStatus.value() == OrderStatus::CANCELLED && request.restock) {
            if (!restockOrderInventory(orderId)) {
                db.rollbackTransaction();
                return false;
            }
        }
        
        // 提交事务
        return db.commitTransaction();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in updateOrderStatus: " << e.what() << std::endl;
        db.rollbackTransaction();
        return false;
    }
}

std::vector<OrderItem> OrderServiceImpl::getOrderItems(int orderId) {
    std::vector<OrderItem> items;
    auto& db = Database::getInstance();
    
    std::stringstream ss;
    ss << "SELECT * FROM order_items WHERE order_id = " << orderId;
    
    auto results = db.query(ss.str());
    for (const auto& row : results) {
        items.push_back(buildOrderItem(row));
    }
    
    return items;
}

bool OrderServiceImpl::validateOrderCreateRequest(const OrderCreateRequest& request) {
    // 检查订单项不能为空
    if (request.items.empty()) {
        return false;
    }
    
    // 检查每个订单项
    for (const auto& item : request.items) {
        if (item.quantity <= 0) {
            return false;
        }
    }
    
    return true;
}

bool OrderServiceImpl::validateStatusTransition(OrderStatus currentStatus, OrderStatus newStatus) {
    // 状态流转规则验证
    switch (currentStatus) {
        case OrderStatus::PENDING:
            // PENDING 可以转到 PAID、CANCELLED 或 SHIPPED
            return newStatus == OrderStatus::PAID || 
                   newStatus == OrderStatus::CANCELLED ||
                   newStatus == OrderStatus::SHIPPED;
        case OrderStatus::PAID:
            // PAID 可以转到 SHIPPED 或 CANCELLED
            return newStatus == OrderStatus::SHIPPED || 
                   newStatus == OrderStatus::CANCELLED;
        case OrderStatus::SHIPPED:
            // SHIPPED 状态不可变
            return newStatus == OrderStatus::SHIPPED;
        case OrderStatus::CANCELLED:
            // CANCELLED 状态不可变
            return newStatus == OrderStatus::CANCELLED;
        default:
            return false;
    }
}

Order OrderServiceImpl::buildOrder(const std::map<std::string, std::string>& data) {
    Order order;
    
    if (data.find("id") != data.end() && !data.at("id").empty()) {
        try {
            order.id = std::stoi(data.at("id"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid order id value: " << data.at("id") << std::endl;
        }
    }
    
    if (data.find("total_amount") != data.end() && !data.at("total_amount").empty()) {
        try {
            order.total_amount = std::stod(data.at("total_amount"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid total_amount value: " << data.at("total_amount") << std::endl;
            order.total_amount = 0.0; // 设置默认值
        }
    }
    
    if (data.find("created_at") != data.end()) order.created_at = data.at("created_at");
    if (data.find("updated_at") != data.end()) order.updated_at = data.at("updated_at");
    
    if (data.find("status") != data.end()) {
        auto status = stringToOrderStatus(data.at("status"));
        if (status) {
            order.status = status.value();
        }
    }
    
    return order;
}

OrderItem OrderServiceImpl::buildOrderItem(const std::map<std::string, std::string>& data) {
    OrderItem item;
    
    if (data.find("id") != data.end()) item.id = std::stoi(data.at("id"));
    if (data.find("order_id") != data.end()) item.order_id = std::stoi(data.at("order_id"));
    if (data.find("product_id") != data.end()) item.product_id = std::stoi(data.at("product_id"));
    if (data.find("quantity") != data.end()) item.quantity = std::stoi(data.at("quantity"));
    if (data.find("unit_price") != data.end()) item.unit_price = std::stod(data.at("unit_price"));
    if (data.find("subtotal") != data.end()) item.subtotal = std::stod(data.at("subtotal"));
    
    return item;
}

bool OrderServiceImpl::restockOrderInventory(int orderId) {
    // 获取订单详情
    auto order = getOrderById(orderId);
    if (!order) {
        return false;
    }
    
    // 回滚每个商品的库存
    for (const auto& item : order->items) {
        if (!productService_->adjustStock(item.product_id, item.quantity, "order_cancelled")) {
            std::cerr << "Failed to restock inventory for product " << item.product_id << std::endl;
            return false;
        }
    }
    
    return true;
}
