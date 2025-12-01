// 乘客数据访问对象
#ifndef RIDER_DAO_H
#define RIDER_DAO_H

#include "../model/entities.h"
#include <vector>
#include <memory>

class RiderDAO {
private:
    DBManager* dbManager;
    
    // 将时间点转换为字符串
    std::string timePointToString(const TimePoint& timePoint);
    
    // 将字符串转换为时间点
    TimePoint stringToTimePoint(const std::string& timeStr);
    
public:
    RiderDAO();
    ~RiderDAO();
    
    // 创建新乘客
    int createRider(const std::string& name, const std::string& phoneNumber = "");
    
    // 根据ID获取乘客
    std::unique_ptr<Rider> getRiderById(int id);
    
    // 更新乘客信息
    bool updateRider(const Rider& rider);
    
    // 更新乘客评分
    bool updateRiderRating(int id, float rating);
    
    // 获取所有乘客
    std::vector<std::unique_ptr<Rider>> getAllRiders();
    
    // 删除乘客
    bool deleteRider(int id);
};

#endif // RIDER_DAO_H
