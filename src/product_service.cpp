#include "product_service.h"
#include "database.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

std::optional<Product> ProductServiceImpl::createProduct(const ProductCreateRequest& request) {
    // 验证请求
    if (!validateProductCreateRequest(request)) {
        return std::nullopt;
    }
    
    auto& db = Database::getInstance();
    
    // 使用预处理语句防止SQL注入
    std::string sql = "INSERT INTO products (name, sku, price, stock, reorder_threshold) VALUES (?, ?, ?, ?, ?);";
    PreparedStatement stmt(db.getConnection(), sql);
    
    if (!stmt.bind(1, request.name) ||
        !stmt.bind(2, request.sku) ||
        !stmt.bind(3, request.price) ||
        !stmt.bind(4, request.initial_stock) ||
        !stmt.bind(5, request.reorder_threshold)) {
        std::cerr << "Failed to bind parameters: " << stmt.getErrorMessage() << std::endl;
        return std::nullopt;
    }
    
    if (!stmt.execute()) {
        std::cerr << "Failed to create product: " << stmt.getErrorMessage() << std::endl;
        return std::nullopt;
    }
    
    // 获取插入的ID
    int64_t productId = db.getLastInsertId();
    
    // 如果初始库存大于0，记录库存日志
    if (request.initial_stock > 0) {
        std::string logSql = "INSERT INTO inventory_logs (product_id, delta, reason, previous_stock, current_stock) VALUES (?, ?, ?, ?, ?);";
        PreparedStatement logStmt(db.getConnection(), logSql);
        logStmt.bind(1, static_cast<int>(productId));
        logStmt.bind(2, request.initial_stock);
        logStmt.bind(3, "initial_stock");
        logStmt.bind(4, 0);
        logStmt.bind(5, request.initial_stock);
        logStmt.execute();
    }
    
    // 返回创建的商品
    return getProductById(static_cast<int>(productId));
}

ProductListResponse ProductServiceImpl::getProducts(const Pagination& pagination) {
    ProductListResponse response;
    auto& db = Database::getInstance();
    
    // 构建查询条件
    std::stringstream ss;
    ss << "SELECT COUNT(*) as count FROM products";
    
    if (!pagination.keyword.empty()) {
        ss << " WHERE name LIKE '%" << pagination.keyword << "%' OR sku LIKE '%" << pagination.keyword << "%'";
    }
    
    // 获取总数
    auto countResult = db.query(ss.str());
    if (!countResult.empty()) {
        response.pagination.total = std::stoi(countResult[0]["count"]);
    }
    
    // 构建分页查询
    ss.str("");
    ss << "SELECT * FROM products";
    
    if (!pagination.keyword.empty()) {
        ss << " WHERE name LIKE '%" << pagination.keyword << "%' OR sku LIKE '%" << pagination.keyword << "%'";
    }
    
    ss << " ORDER BY created_at DESC LIMIT " << pagination.page_size << " OFFSET " << pagination.getOffset();
    
    auto results = db.query(ss.str());
    
    // 构建商品列表
    for (const auto& row : results) {
        response.products.push_back(buildProduct(row));
    }
    
    // 设置分页信息
    response.pagination.page = pagination.page;
    response.pagination.page_size = pagination.page_size;
    response.pagination.calculateTotalPages();
    
    return response;
}

std::optional<Product> ProductServiceImpl::getProductById(int id) {
    auto& db = Database::getInstance();
    std::stringstream ss;
    ss << "SELECT * FROM products WHERE id = " << id;
    
    auto results = db.query(ss.str());
    if (results.empty()) {
        return std::nullopt;
    }
    
    return buildProduct(results[0]);
}

std::optional<Product> ProductServiceImpl::updateProduct(int id, const ProductUpdateRequest& request) {
    // 验证请求
    if (!validateProductUpdateRequest(request)) {
        return std::nullopt;
    }
    
    // 检查商品是否存在
    auto existingProduct = getProductById(id);
    if (!existingProduct) {
        return std::nullopt;
    }
    
    auto& db = Database::getInstance();
    
    // 构建更新语句
    std::stringstream ss;
    ss << "UPDATE products SET updated_at = CURRENT_TIMESTAMP";
    
    if (request.name.has_value()) {
        ss << ", name = '" << request.name.value() << "'";
    }
    
    if (request.price.has_value()) {
        ss << ", price = " << request.price.value();
    }
    
    if (request.reorder_threshold.has_value()) {
        ss << ", reorder_threshold = " << request.reorder_threshold.value();
    }
    
    ss << " WHERE id = " << id;
    
    if (!db.execute(ss.str())) {
        std::cerr << "Failed to update product: " << db.getErrorMessage() << std::endl;
        return std::nullopt;
    }
    
    // 返回更新后的商品
    return getProductById(id);
}

bool ProductServiceImpl::adjustStock(int productId, int delta, const std::string& reason) {
    auto& db = Database::getInstance();
    bool usingExternalTransaction = true; // 假设已经在外部事务中
    
    // 如果需要单独使用，这里可以开始事务，但通常在调用方已经开始了事务
    // 为了避免嵌套事务问题，我们不再在这里开始事务
    
    try {
        // 获取当前库存
        auto product = getProductById(productId);
        if (!product) {
            std::cerr << "Product not found in adjustStock: " << productId << std::endl;
            if (!usingExternalTransaction) {
                db.rollbackTransaction();
            }
            return false;
        }
        
        int newStock = product->stock + delta;
        std::cerr << "Adjusting stock for product " << productId << ": current=" << product->stock 
                  << ", delta=" << delta << ", new=" << newStock << std::endl;
        
        // 检查库存不能为负
        if (newStock < 0) {
            std::cerr << "Insufficient stock: newStock=" << newStock << " < 0" << std::endl;
            if (!usingExternalTransaction) {
                db.rollbackTransaction();
            }
            return false;
        }
        
        // 更新库存
        std::stringstream updateSql;
        updateSql << "UPDATE products SET stock = " << newStock << ", updated_at = CURRENT_TIMESTAMP WHERE id = " << productId;
        
        if (!db.execute(updateSql.str())) {
            std::cerr << "Failed to execute updateSql: " << db.getErrorMessage() << std::endl;
            if (!usingExternalTransaction) {
                db.rollbackTransaction();
            }
            return false;
        }
        
        // 记录库存变动日志
        std::string logSql = "INSERT INTO inventory_logs (product_id, delta, reason, previous_stock, current_stock) VALUES (?, ?, ?, ?, ?);";
        PreparedStatement logStmt(db.getConnection(), logSql);
        logStmt.bind(1, productId);
        logStmt.bind(2, delta);
        logStmt.bind(3, reason);
        logStmt.bind(4, product->stock);
        logStmt.bind(5, newStock);
        
        if (!logStmt.execute()) {
            std::cerr << "Failed to insert inventory log: " << logStmt.getErrorMessage() << std::endl;
            if (!usingExternalTransaction) {
                db.rollbackTransaction();
            }
            return false;
        }
        
        // 不再在这里提交事务，由外部事务处理
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in adjustStock: " << e.what() << std::endl;
        if (!usingExternalTransaction) {
            db.rollbackTransaction();
        }
        return false;
    }
    catch (...) {
        std::cerr << "Unknown exception in adjustStock" << std::endl;
        if (!usingExternalTransaction) {
            db.rollbackTransaction();
        }
        return false;
    }
}

std::vector<Product> ProductServiceImpl::getLowStockProducts() {
    std::vector<Product> products;
    auto& db = Database::getInstance();
    
    std::string sql = "SELECT * FROM products WHERE stock <= reorder_threshold";
    auto results = db.query(sql);
    
    for (const auto& row : results) {
        products.push_back(buildProduct(row));
    }
    
    return products;
}

std::vector<Product> ProductServiceImpl::getProductsByIds(const std::vector<int>& ids) {
    std::vector<Product> products;
    
    if (ids.empty()) {
        return products;
    }
    
    auto& db = Database::getInstance();
    std::stringstream ss;
    ss << "SELECT * FROM products WHERE id IN (";
    
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << ids[i];
    }
    ss << ")";
    
    auto results = db.query(ss.str());
    
    for (const auto& row : results) {
        products.push_back(buildProduct(row));
    }
    
    return products;
}

bool ProductServiceImpl::validateProductCreateRequest(const ProductCreateRequest& request) {
    // 验证必填字段
    if (request.name.empty() || request.sku.empty()) {
        return false;
    }
    
    // 验证价格不能为负
    if (request.price < 0) {
        return false;
    }
    
    // 验证初始库存不能为负
    if (request.initial_stock < 0) {
        return false;
    }
    
    // 验证阈值不能为负
    if (request.reorder_threshold < 0) {
        return false;
    }
    
    // 检查SKU是否已存在
    auto& db = Database::getInstance();
    std::stringstream ss;
    ss << "SELECT COUNT(*) as count FROM products WHERE sku = '" << request.sku << "'";
    auto results = db.query(ss.str());
    
    if (!results.empty() && std::stoi(results[0]["count"]) > 0) {
        return false;
    }
    
    return true;
}

bool ProductServiceImpl::validateProductUpdateRequest(const ProductUpdateRequest& request) {
    // 验证价格
    if (request.price.has_value() && request.price.value() < 0) {
        return false;
    }
    
    // 验证阈值
    if (request.reorder_threshold.has_value() && request.reorder_threshold.value() < 0) {
        return false;
    }
    
    return true;
}

Product ProductServiceImpl::buildProduct(const std::map<std::string, std::string>& data) {
    Product product;
    
    if (data.find("id") != data.end() && !data.at("id").empty()) {
        try {
            product.id = std::stoi(data.at("id"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid id value: " << data.at("id") << std::endl;
        }
    }
    
    if (data.find("name") != data.end()) product.name = data.at("name");
    if (data.find("sku") != data.end()) product.sku = data.at("sku");
    
    if (data.find("price") != data.end() && !data.at("price").empty()) {
        try {
            product.price = std::stod(data.at("price"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid price value: " << data.at("price") << std::endl;
        }
    }
    
    if (data.find("stock") != data.end() && !data.at("stock").empty()) {
        try {
            product.stock = std::stoi(data.at("stock"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid stock value: " << data.at("stock") << std::endl;
            product.stock = 0; // 设置默认值
        }
    }
    
    if (data.find("reorder_threshold") != data.end() && !data.at("reorder_threshold").empty()) {
        try {
            product.reorder_threshold = std::stoi(data.at("reorder_threshold"));
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid reorder_threshold value: " << data.at("reorder_threshold") << std::endl;
        }
    }
    
    if (data.find("created_at") != data.end()) product.created_at = data.at("created_at");
    if (data.find("updated_at") != data.end()) product.updated_at = data.at("updated_at");
    
    return product;
}
