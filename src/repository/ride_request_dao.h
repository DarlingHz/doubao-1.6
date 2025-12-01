// 出行请求数据访问对象
#ifndef RIDE_REQUEST_DAO_H
#define RIDE_REQUEST_DAO_H

#include "../model/entities.h"
#include <vector>
#include <memory>

class RideRequestDAO {
private:
    DBManager* dbManager;
    
    // 将时间点转换为字符串
    std::string timePointToString(const TimePoint& timePoint);
    
    // 将字符串转换为时间点
    TimePoint stringToTimePoint(const std::string& timeStr);
    
    // 将RideRequestStatus转换为整数
    int rideRequestStatusToInt(RideRequestStatus status);
    
    // 将整数转换为RideRequestStatus
    RideRequestStatus intToRideRequestStatus(int statusInt);
    
public:
    RideRequestDAO();
    ~RideRequestDAO();
    
    // 创建新的出行请求
    int createRideRequest(int riderId, const Location& start, const Location& end,
                         TimePoint earliestTime, TimePoint latestTime);
    
    // 根据ID获取出行请求
    std::unique_ptr<RideRequest> getRideRequestById(int id);
    
    // 更新出行请求
    bool updateRideRequest(const RideRequest& request);
    
    // 更新出行请求状态
    bool updateRideRequestStatus(int id, RideRequestStatus status);
    
    // 获取所有待匹配的出行请求
    std::vector<std::unique_ptr<RideRequest>> getPendingRideRequests();
    
    // 根据乘客ID获取出行请求
    std::vector<std::unique_ptr<RideRequest>> getRideRequestsByRiderId(int riderId);
    
    // 根据位置范围获取待匹配的出行请求
    std::vector<std::unique_ptr<RideRequest>> getPendingRideRequestsInRange(const Location& center, int maxDistance);
    
    // 获取所有出行请求
    std::vector<std::unique_ptr<RideRequest>> getAllRideRequests();
    
    // 删除出行请求
    bool deleteRideRequest(int id);
};

#endif // RIDE_REQUEST_DAO_H
