// 行程数据访问对象
#ifndef TRIP_DAO_H
#define TRIP_DAO_H

#include "../model/entities.h"
#include <vector>
#include <memory>

class TripDAO {
private:
    DBManager* dbManager;
    
    // 将时间点转换为字符串
    std::string timePointToString(const TimePoint& timePoint);
    
    // 将字符串转换为时间点
    TimePoint stringToTimePoint(const std::string& timeStr);
    
    // 将TripStatus转换为整数
    int tripStatusToInt(TripStatus status);
    
    // 将整数转换为TripStatus
    TripStatus intToTripStatus(int statusInt);
    
public:
    TripDAO();
    ~TripDAO();
    
    // 创建新行程
    int createTrip(int driverId, int riderId, int rideRequestId);
    
    // 根据ID获取行程
    std::unique_ptr<Trip> getTripById(int id);
    
    // 更新行程
    bool updateTrip(const Trip& trip);
    
    // 更新行程状态
    bool updateTripStatus(int id, TripStatus status);
    
    // 获取司机的所有行程
    std::vector<std::unique_ptr<Trip>> getTripsByDriverId(int driverId);
    
    // 获取乘客的所有行程
    std::vector<std::unique_ptr<Trip>> getTripsByRiderId(int riderId);
    
    // 获取进行中的行程
    std::vector<std::unique_ptr<Trip>> getActiveTrips();
    
    // 根据出行请求ID获取行程
    std::unique_ptr<Trip> getTripByRideRequestId(int rideRequestId);
    
    // 获取所有行程
    std::vector<std::unique_ptr<Trip>> getAllTrips();
    
    // 删除行程
    bool deleteTrip(int id);
};

#endif // TRIP_DAO_H
