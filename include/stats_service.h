#ifndef STATS_SERVICE_H
#define STATS_SERVICE_H

#include "models.h"
#include "product_service.h"
#include "order_service.h"
#include <memory>
#include <string>

// 统计服务接口
class StatsService {
public:
    virtual ~StatsService() = default;
    
    // 获取低库存商品统计
    virtual std::vector<Product> getLowStockProducts() = 0;
    
    // 获取每日订单统计
    virtual std::optional<DailySummaryResponse> getDailySummary(const std::string& date) = 0;
};

// 统计服务实现类
class StatsServiceImpl : public StatsService {
public:
    StatsServiceImpl(std::shared_ptr<ProductService> productService, 
                    std::shared_ptr<OrderService> orderService);
    ~StatsServiceImpl() = default;
    
    std::vector<Product> getLowStockProducts() override;
    std::optional<DailySummaryResponse> getDailySummary(const std::string& date) override;
    
private:
    std::shared_ptr<ProductService> productService_;
    std::shared_ptr<OrderService> orderService_;
    
    // 验证日期格式
    bool validateDateFormat(const std::string& date);
};

#endif // STATS_SERVICE_H
