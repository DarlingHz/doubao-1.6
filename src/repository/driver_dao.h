// 车主数据访问对象
#ifndef DRIVER_DAO_H
#define DRIVER_DAO_H

#include "../model/entities.h"
#include <vector>
#include <memory>

class DriverDAO {
private:
    DBManager* dbManager;
    
    // 将时间点转换为字符串
    std::string timePointToString(const TimePoint& timePoint);
    
    // 将字符串转换为时间点
    TimePoint stringToTimePoint(const std::string& timeStr);
    
    // 将DriverStatus转换为整数
    int driverStatusToInt(DriverStatus status);
    
    // 将整数转换为DriverStatus
    DriverStatus intToDriverStatus(int statusInt);
    
public:
    DriverDAO();
    ~DriverDAO();
    
    // 创建新车主
    int createDriver(const std::string& name, const std::string& licensePlate, 
                    const std::string& carModel, int capacity);
    
    // 根据ID获取车主
    std::unique_ptr<Driver> getDriverById(int id);
    
    // 更新车主信息
    bool updateDriver(const Driver& driver);
    
    // 更新车主状态
    bool updateDriverStatus(int id, DriverStatus status);
    
    // 更新车主位置
    bool updateDriverLocation(int id, const Location& location);
    
    // 更新车主评分
    bool updateDriverRating(int id, float rating);
    
    // 获取所有可用车主
    std::vector<std::unique_ptr<Driver>> getAvailableDrivers();
    
    // 根据位置范围获取可用车主
    std::vector<std::unique_ptr<Driver>> getAvailableDriversInRange(const Location& center, int maxDistance);
    
    // 更新活跃行程数
    bool updateActiveTripsCount(int id, int count);
    
    // 获取所有车主
    std::vector<std::unique_ptr<Driver>> getAllDrivers();
    
    // 删除车主
    bool deleteDriver(int id);
};

#endif // DRIVER_DAO_H
