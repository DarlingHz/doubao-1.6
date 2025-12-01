// 核心实体定义
#ifndef ENTITIES_H
#define ENTITIES_H

#include <string>
#include <chrono>
#include <memory>

// 时间类型定义
typedef std::chrono::system_clock::time_point TimePoint;
typedef std::chrono::seconds Seconds;

// 坐标结构
struct Location {
    int x;  // 可以是经度的整数表示
    int y;  // 可以是纬度的整数表示
    
    Location(int x = 0, int y = 0) : x(x), y(y) {}
    
    // 计算曼哈顿距离
    int manhattanDistance(const Location& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }
    
    // 计算欧几里得距离（平方，避免开根号开销）
    int euclideanDistanceSquared(const Location& other) const {
        int dx = x - other.x;
        int dy = y - other.y;
        return dx * dx + dy * dy;
    }
};

// 车主状态枚举
enum class DriverStatus {
    OFFLINE,    // 离线
    AVAILABLE,  // 在线可接单
    ON_TRIP     // 行程中
};

// 出行请求状态枚举
enum class RideRequestStatus {
    PENDING,    // 待匹配
    MATCHED,    // 已匹配
    CANCELLED,  // 已取消
    COMPLETED   // 已完成
};

// 行程状态枚举
enum class TripStatus {
    ONGOING,    // 进行中
    COMPLETED,  // 已完成
    CANCELLED   // 已取消
};

// 乘客类
class Rider {
private:
    int id;
    std::string name;
    std::string phoneNumber;  // 可选
    float rating;             // 评分
    TimePoint registrationTime;
    
public:
    Rider(int id, const std::string& name, const std::string& phoneNumber = "")
        : id(id), name(name), phoneNumber(phoneNumber), rating(5.0f), 
          registrationTime(std::chrono::system_clock::now()) {}
    
    // Getters and setters
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getPhoneNumber() const { return phoneNumber; }
    float getRating() const { return rating; }
    TimePoint getRegistrationTime() const { return registrationTime; }
    
    void setRating(float rating) { this->rating = rating; }
    void setPhoneNumber(const std::string& phoneNumber) { this->phoneNumber = phoneNumber; }
};

// 车主类
class Driver {
private:
    int id;
    std::string name;
    std::string licensePlate;  // 车牌号
    std::string carModel;      // 车型
    int capacity;              // 可载人数
    DriverStatus status;
    float rating;
    Location currentLocation;
    TimePoint registrationTime;
    int activeTripsCount;  // 当前活跃行程数
    
public:
    Driver(int id, const std::string& name, const std::string& licensePlate, 
           const std::string& carModel, int capacity)
        : id(id), name(name), licensePlate(licensePlate), carModel(carModel), 
          capacity(capacity), status(DriverStatus::OFFLINE), rating(5.0f),
          registrationTime(std::chrono::system_clock::now()), activeTripsCount(0) {}
    
    // Getters and setters
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getLicensePlate() const { return licensePlate; }
    const std::string& getCarModel() const { return carModel; }
    int getCapacity() const { return capacity; }
    DriverStatus getStatus() const { return status; }
    float getRating() const { return rating; }
    const Location& getCurrentLocation() const { return currentLocation; }
    TimePoint getRegistrationTime() const { return registrationTime; }
    int getActiveTripsCount() const { return activeTripsCount; }
    
    void setStatus(DriverStatus status) { this->status = status; }
    void setRating(float rating) { this->rating = rating; }
    void setCurrentLocation(const Location& location) { this->currentLocation = location; }
    void incrementActiveTrips() { activeTripsCount++; }
    void decrementActiveTrips() { if (activeTripsCount > 0) activeTripsCount--; }
    
    // 判断是否可用（在线且可以接单）
    bool isAvailable() const {
        return status == DriverStatus::AVAILABLE;
    }
};

// 出行请求类
class RideRequest {
private:
    int id;
    int riderId;
    Location startLocation;
    Location endLocation;
    TimePoint earliestDepartureTime;
    TimePoint latestDepartureTime;
    RideRequestStatus status;
    TimePoint creationTime;
    
public:
    RideRequest(int id, int riderId, const Location& start, const Location& end,
                TimePoint earliestTime, TimePoint latestTime)
        : id(id), riderId(riderId), startLocation(start), endLocation(end),
          earliestDepartureTime(earliestTime), latestDepartureTime(latestTime),
          status(RideRequestStatus::PENDING), creationTime(std::chrono::system_clock::now()) {}
    
    // Getters and setters
    int getId() const { return id; }
    int getRiderId() const { return riderId; }
    const Location& getStartLocation() const { return startLocation; }
    const Location& getEndLocation() const { return endLocation; }
    TimePoint getEarliestDepartureTime() const { return earliestDepartureTime; }
    TimePoint getLatestDepartureTime() const { return latestDepartureTime; }
    RideRequestStatus getStatus() const { return status; }
    TimePoint getCreationTime() const { return creationTime; }
    
    void setStatus(RideRequestStatus status) { this->status = status; }
    
    // 判断请求是否在时间窗口内有效
    bool isTimeWindowValid() const {
        TimePoint now = std::chrono::system_clock::now();
        return now >= earliestDepartureTime && now <= latestDepartureTime;
    }
};

// 行程类
class Trip {
private:
    int id;
    int driverId;
    int riderId;
    int rideRequestId;
    TimePoint matchTime;
    TripStatus status;
    
public:
    Trip(int id, int driverId, int riderId, int rideRequestId)
        : id(id), driverId(driverId), riderId(riderId), rideRequestId(rideRequestId),
          matchTime(std::chrono::system_clock::now()), status(TripStatus::ONGOING) {}
    
    // Getters and setters
    int getId() const { return id; }
    int getDriverId() const { return driverId; }
    int getRiderId() const { return riderId; }
    int getRideRequestId() const { return rideRequestId; }
    TimePoint getMatchTime() const { return matchTime; }
    TripStatus getStatus() const { return status; }
    
    void setStatus(TripStatus status) { this->status = status; }
};

#endif // ENTITIES_H
