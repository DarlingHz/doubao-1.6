// 匹配服务类 - 实现核心的顺风车匹配算法
#ifndef MATCHING_SERVICE_H
#define MATCHING_SERVICE_H

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

// 前向声明
class RideRequest;
class Driver;
class RideRequestDAO;
class DriverDAO;
class TripDAO;
class SpatialIndex;
class StatsMonitor;

class MatchingService {
private:
    static MatchingService* instance;
    
    // DAO 依赖
    std::unique_ptr<RideRequestDAO> rideRequestDAO;
    std::unique_ptr<DriverDAO> driverDAO;
    std::unique_ptr<TripDAO> tripDAO;
    
    // 空间索引用于快速位置查询
    std::unique_ptr<SpatialIndex> spatialIndex;
    
    // 统计监控
    StatsMonitor* statsMonitor;
    
    // 配置参数
    int maxMatchDistance;  // 最大匹配距离（曼哈顿距离）
    int gridSize;          // 网格大小
    
    // 线程安全
    std::mutex matchMutex;
    std::condition_variable matchCondition;
    bool stopRequested;    // 停止标志
    
    // 私有构造函数
    MatchingService();
    
    // 计算曼哈顿距离
    int calculateManhattanDistance(int startX, int startY, int endX, int endY);
    
    // 执行单次匹配尝试
    bool tryMatchRequest(const RideRequest& request);
    
    // 匹配算法实现
    Driver* findBestDriver(const RideRequest& request, const std::vector<Driver*>& nearbyDrivers);
    
    // 处理匹配成功
    bool handleMatchSuccess(const RideRequest& request, Driver* driver);
    
public:
    static MatchingService* getInstance();
    
    // 初始化服务
    void initialize(int maxDistance = 5000, int gridSize = 1000);
    
    // 启动匹配服务线程
    void start();
    
    // 停止匹配服务线程
    void stop();
    
    // 手动触发匹配（新请求或车主上线时调用）
    void triggerMatching();
    
    // 尝试匹配单个请求
    bool matchRequest(int requestId);
    
    // 当车主状态或位置更新时调用
    void onDriverUpdate(int driverId, bool locationChanged);
    
    // 获取待匹配请求数量
    int getPendingRequestsCount();
    
    // 匹配服务线程函数
    void matchingThreadFunction();
};

#endif // MATCHING_SERVICE_H
