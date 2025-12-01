// 用户服务类 - 管理乘客和车主的核心业务逻辑
#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <string>
#include <memory>
#include "../model/entities.h"

// 前向声明
class RiderDAO;
class DriverDAO;
class RideRequestDAO;
class TripDAO;
class StatsMonitor;
class SpatialIndex;
class MatchingService;

class UserService {
private:
    static UserService* instance;
    
    // DAO 依赖
    std::unique_ptr<RiderDAO> riderDAO;
    std::unique_ptr<DriverDAO> driverDAO;
    std::unique_ptr<RideRequestDAO> rideRequestDAO;
    std::unique_ptr<TripDAO> tripDAO;
    
    // 其他服务依赖
    StatsMonitor* statsMonitor;
    SpatialIndex* spatialIndex;
    MatchingService* matchingService;
    
    // 线程安全锁
    std::mutex matchMutex;
    
    // 私有构造函数
    UserService();
    
    // 验证手机号格式
    bool isValidPhoneNumber(const std::string& phoneNumber);
    
    // 验证车辆信息
    bool isValidVehicleInfo(const Vehicle& vehicle);
    
public:
    static UserService* getInstance();
    
    // 初始化服务
    void initialize();
    
    // 乘客相关方法
    Rider* createRider(const std::string& name, const std::string& phoneNumber = "");
    Rider* getRiderById(int riderId);
    bool updateRider(int riderId, const std::string& name, const std::string& phoneNumber = "");
    bool deleteRider(int riderId);
    bool updateRiderRating(int riderId, float newRating);
    
    // 车主相关方法
    Driver* createDriver(const std::string& name, const Vehicle& vehicle, const std::string& phoneNumber = "");
    Driver* getDriverById(int driverId);
    bool updateDriver(int driverId, const std::string& name, const Vehicle& vehicle, const std::string& phoneNumber = "");
    bool updateDriverStatus(int driverId, DriverStatus status);
    bool updateDriverLocation(int driverId, int x, int y);
    bool updateDriverRating(int driverId, float newRating);
    bool deleteDriver(int driverId);
    
    // 出行请求相关方法
    RideRequest* createRideRequest(int riderId, const Location& startLocation, 
                                  const Location& endLocation, time_t earliestDeparture, 
                                  time_t latestDeparture);
    RideRequest* getRideRequestById(int requestId);
    bool cancelRideRequest(int requestId);
    
    // 行程相关方法
    Trip* getTripById(int tripId);
    std::vector<Trip*> getTripsByDriver(int driverId);
    std::vector<Trip*> getTripsByRider(int riderId);
    bool startTrip(int tripId);
    bool completeTrip(int tripId, float driverRating, float riderRating);
    bool cancelTrip(int tripId);
    
    // 批量获取方法（用于统计等）
    std::vector<Rider*> getAllRiders();
    std::vector<Driver*> getAllDrivers();
    std::vector<Driver*> getAvailableDrivers();
};

#endif // USER_SERVICE_H
