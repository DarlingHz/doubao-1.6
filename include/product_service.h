#ifndef PRODUCT_SERVICE_H
#define PRODUCT_SERVICE_H

#include "models.h"
#include "database.h"
#include <string>
#include <vector>
#include <map>

// 商品服务接口
class ProductService {
public:
    virtual ~ProductService() = default;
    
    // 创建商品
    virtual std::optional<Product> createProduct(const ProductCreateRequest& request) = 0;
    
    // 获取商品列表（分页）
    virtual ProductListResponse getProducts(const Pagination& pagination) = 0;
    
    // 获取单个商品
    virtual std::optional<Product> getProductById(int id) = 0;
    
    // 更新商品信息
    virtual std::optional<Product> updateProduct(int id, const ProductUpdateRequest& request) = 0;
    
    // 调整商品库存
    virtual bool adjustStock(int productId, int delta, const std::string& reason) = 0;
    
    // 获取库存低于阈值的商品
    virtual std::vector<Product> getLowStockProducts() = 0;
    
    // 批量获取商品信息（用于订单创建时的库存检查）
    virtual std::vector<Product> getProductsByIds(const std::vector<int>& ids) = 0;
};

// 商品服务实现类
class ProductServiceImpl : public ProductService {
public:
    ProductServiceImpl() = default;
    ~ProductServiceImpl() = default;
    
    std::optional<Product> createProduct(const ProductCreateRequest& request) override;
    ProductListResponse getProducts(const Pagination& pagination) override;
    std::optional<Product> getProductById(int id) override;
    std::optional<Product> updateProduct(int id, const ProductUpdateRequest& request) override;
    bool adjustStock(int productId, int delta, const std::string& reason) override;
    std::vector<Product> getLowStockProducts() override;
    std::vector<Product> getProductsByIds(const std::vector<int>& ids) override;
    
private:
    // 验证商品创建请求
    bool validateProductCreateRequest(const ProductCreateRequest& request);
    
    // 验证商品更新请求
    bool validateProductUpdateRequest(const ProductUpdateRequest& request);
    
    // 构建商品对象
    Product buildProduct(const std::map<std::string, std::string>& data);
};

#endif // PRODUCT_SERVICE_H
