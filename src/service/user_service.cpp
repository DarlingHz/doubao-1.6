// 用户服务实现
#include "user_service.h"
#include "../repository/rider_dao.h"
#include "../repository/driver_dao.h"
#include "../repository/ride_request_dao.h"
#include "../repository/trip_dao.h"
#include "../utils/logger.h"
#include "../utils/stats_monitor.h"
#include "../utils/spatial_index.h"
#include "matching_service.h"
#include <regex>

UserService* UserService::instance = nullptr;

UserService::UserService() {
    // 初始化 DAO
    riderDAO = std::make_unique<RiderDAO>();
    driverDAO = std::make_unique<DriverDAO>();
    rideRequestDAO = std::make_unique<RideRequestDAO>();
    tripDAO = std::make_unique<TripDAO>();
    
    // 获取其他服务实例
    statsMonitor = StatsMonitor::getInstance();
    spatialIndex = SpatialIndex::getInstance();
    matchingService = MatchingService::getInstance();
}

UserService* UserService::getInstance() {
    static std::mutex mutex;
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new UserService();
        }
    }
    return instance;
}

void UserService::initialize() {
    Logger::getInstance()->info("UserService initialized");
    
    // 初始化空间索引中的在线司机数据
    std::vector<Driver*> availableDrivers = driverDAO->getAvailableDrivers();
    for (Driver* driver : availableDrivers) {
        spatialIndex->addDriver(driver->getId(), driver->getLocation().x, driver->getLocation().y);
        delete driver;
    }
    
    // 更新统计信息
    int onlineCount = driverDAO->getOnlineDriversCount();
    int availableCount = availableDrivers.size();
    int pendingCount = rideRequestDAO->getPendingRequestsCount();
    int activeTrips = tripDAO->getActiveTripsCount();
    
    for (int i = 0; i < onlineCount; i++) {
        statsMonitor->incrementOnlineDrivers();
    }
    for (int i = 0; i < availableCount; i++) {
        statsMonitor->incrementAvailableDrivers();
    }
    for (int i = 0; i < pendingCount; i++) {
        statsMonitor->incrementPendingRequests();
    }
    for (int i = 0; i < activeTrips; i++) {
        statsMonitor->incrementActiveTrips();
    }
}

bool UserService::isValidPhoneNumber(const std::string& phoneNumber) {
    if (phoneNumber.empty()) {
        return true;  // 手机号可选
    }
    
    // 简单的手机号格式验证（中国大陆手机号格式）
    std::regex phoneRegex("^1[3-9]\\d{9}$");
    return std::regex_match(phoneNumber, phoneRegex);
}

bool UserService::isValidVehicleInfo(const Vehicle& vehicle) {
    // 验证车牌号（简单验证，实际应该更严格）
    if (vehicle.licensePlate.empty() || vehicle.licensePlate.length() < 5) {
        return false;
    }
    
    // 验证车型
    if (vehicle.model.empty()) {
        return false;
    }
    
    // 验证可载人数
    if (vehicle.capacity <= 0 || vehicle.capacity > 7) {
        return false;
    }
    
    return true;
}

// 乘客相关方法实现
Rider* UserService::createRider(const std::string& name, const std::string& phoneNumber) {
    if (name.empty()) {
        Logger::getInstance()->error("Failed to create rider: name cannot be empty");
        return nullptr;
    }
    
    if (!isValidPhoneNumber(phoneNumber)) {
        Logger::getInstance()->error("Failed to create rider: invalid phone number format");
        return nullptr;
    }
    
    Rider rider;
    rider.setName(name);
    rider.setPhoneNumber(phoneNumber);
    rider.setRating(5.0f);  // 默认评分
    rider.setRegistrationTime(time(nullptr));
    
    int riderId = riderDAO->create(rider);
    if (riderId <= 0) {
        Logger::getInstance()->error("Failed to create rider in database");
        return nullptr;
    }
    
    Logger::getInstance()->info("Created new rider with id: " + std::to_string(riderId));
    return riderDAO->getById(riderId);
}

Rider* UserService::getRiderById(int riderId) {
    return riderDAO->getById(riderId);
}

bool UserService::updateRider(int riderId, const std::string& name, const std::string& phoneNumber) {
    Rider* existingRider = riderDAO->getById(riderId);
    if (!existingRider) {
        Logger::getInstance()->error("Failed to update rider: rider not found with id " + std::to_string(riderId));
        return false;
    }
    
    if (name.empty()) {
        delete existingRider;
        Logger::getInstance()->error("Failed to update rider: name cannot be empty");
        return false;
    }
    
    if (!isValidPhoneNumber(phoneNumber)) {
        delete existingRider;
        Logger::getInstance()->error("Failed to update rider: invalid phone number format");
        return false;
    }
    
    existingRider->setName(name);
    existingRider->setPhoneNumber(phoneNumber);
    
    bool success = riderDAO->update(*existingRider);
    delete existingRider;
    
    if (success) {
        Logger::getInstance()->info("Updated rider with id: " + std::to_string(riderId));
    } else {
        Logger::getInstance()->error("Failed to update rider in database");
    }
    
    return success;
}

bool UserService::deleteRider(int riderId) {
    // 检查是否有未完成的行程
    std::vector<Trip*> activeTrips = tripDAO->getActiveTripsByRider(riderId);
    if (!activeTrips.empty()) {
        for (Trip* trip : activeTrips) {
            delete trip;
        }
        Logger::getInstance()->error("Failed to delete rider: has active trips");
        return false;
    }
    
    bool success = riderDAO->deleteById(riderId);
    
    if (success) {
        Logger::getInstance()->info("Deleted rider with id: " + std::to_string(riderId));
    } else {
        Logger::getInstance()->error("Failed to delete rider from database");
    }
    
    return success;
}

bool UserService::updateRiderRating(int riderId, float newRating) {
    if (newRating < 1.0f || newRating > 5.0f) {
        Logger::getInstance()->error("Failed to update rider rating: rating must be between 1.0 and 5.0");
        return false;
    }
    
    bool success = riderDAO->updateRating(riderId, newRating);
    
    if (success) {
        Logger::getInstance()->info("Updated rider rating for id " + std::to_string(riderId) + 
                                  " to " + std::to_string(newRating));
    }
    
    return success;
}

// 车主相关方法实现
Driver* UserService::createDriver(const std::string& name, const Vehicle& vehicle, const std::string& phoneNumber) {
    if (name.empty()) {
        Logger::getInstance()->error("Failed to create driver: name cannot be empty");
        return nullptr;
    }
    
    if (!isValidPhoneNumber(phoneNumber)) {
        Logger::getInstance()->error("Failed to create driver: invalid phone number format");
        return nullptr;
    }
    
    if (!isValidVehicleInfo(vehicle)) {
        Logger::getInstance()->error("Failed to create driver: invalid vehicle information");
        return nullptr;
    }
    
    Driver driver;
    driver.setName(name);
    driver.setVehicle(vehicle);
    driver.setPhoneNumber(phoneNumber);
    driver.setStatus(DriverStatus::OFFLINE);
    driver.setRating(5.0f);  // 默认评分
    driver.setRegistrationTime(time(nullptr));
    
    int driverId = driverDAO->create(driver);
    if (driverId <= 0) {
        Logger::getInstance()->error("Failed to create driver in database");
        return nullptr;
    }
    
    Logger::getInstance()->info("Created new driver with id: " + std::to_string(driverId));
    return driverDAO->getById(driverId);
}

Driver* UserService::getDriverById(int driverId) {
    return driverDAO->getById(driverId);
}

bool UserService::updateDriver(int driverId, const std::string& name, const Vehicle& vehicle, const std::string& phoneNumber) {
    Driver* existingDriver = driverDAO->getById(driverId);
    if (!existingDriver) {
        Logger::getInstance()->error("Failed to update driver: driver not found with id " + std::to_string(driverId));
        return false;
    }
    
    if (name.empty()) {
        delete existingDriver;
        Logger::getInstance()->error("Failed to update driver: name cannot be empty");
        return false;
    }
    
    if (!isValidPhoneNumber(phoneNumber)) {
        delete existingDriver;
        Logger::getInstance()->error("Failed to update driver: invalid phone number format");
        return false;
    }
    
    if (!isValidVehicleInfo(vehicle)) {
        delete existingDriver;
        Logger::getInstance()->error("Failed to update driver: invalid vehicle information");
        return false;
    }
    
    existingDriver->setName(name);
    existingDriver->setVehicle(vehicle);
    existingDriver->setPhoneNumber(phoneNumber);
    
    bool success = driverDAO->update(*existingDriver);
    delete existingDriver;
    
    if (success) {
        Logger::getInstance()->info("Updated driver with id: " + std::to_string(driverId));
    } else {
        Logger::getInstance()->error("Failed to update driver in database");
    }
    
    return success;
}

bool UserService::updateDriverStatus(int driverId, DriverStatus status) {
    Driver* existingDriver = driverDAO->getById(driverId);
    if (!existingDriver) {
        Logger::getInstance()->error("Failed to update driver status: driver not found");
        return false;
    }
    
    DriverStatus oldStatus = existingDriver->getStatus();
    
    // 不允许直接从 ON_TRIP 变为 AVAILABLE，必须通过行程完成/取消
    if (oldStatus == DriverStatus::ON_TRIP && status == DriverStatus::AVAILABLE) {
        delete existingDriver;
        Logger::getInstance()->error("Failed to update driver status: cannot directly change from ON_TRIP to AVAILABLE");
        return false;
    }
    
    bool success = driverDAO->updateStatus(driverId, status);
    
    if (success) {
        // 更新统计信息
        if (oldStatus == DriverStatus::OFFLINE && status == DriverStatus::AVAILABLE) {
            statsMonitor->incrementOnlineDrivers();
            statsMonitor->incrementAvailableDrivers();
            // 如果有位置信息，添加到空间索引
            if (existingDriver->getLocation().x != 0 || existingDriver->getLocation().y != 0) {
                spatialIndex->addDriver(driverId, existingDriver->getLocation().x, existingDriver->getLocation().y);
            }
            // 触发匹配
            matchingService->triggerMatching();
        } else if (oldStatus == DriverStatus::AVAILABLE && status == DriverStatus::OFFLINE) {
            statsMonitor->decrementOnlineDrivers();
            statsMonitor->decrementAvailableDrivers();
            spatialIndex->removeDriver(driverId);
        } else if (oldStatus == DriverStatus::AVAILABLE && status == DriverStatus::ON_TRIP) {
            statsMonitor->decrementAvailableDrivers();
            spatialIndex->removeDriver(driverId);
        } else if (oldStatus == DriverStatus::ON_TRIP && status == DriverStatus::OFFLINE) {
            statsMonitor->decrementOnlineDrivers();
            spatialIndex->removeDriver(driverId);
        }
        
        Logger::getInstance()->info("Updated driver " + std::to_string(driverId) + 
                                  " status from " + std::to_string(static_cast<int>(oldStatus)) + 
                                  " to " + std::to_string(static_cast<int>(status)));
    }
    
    delete existingDriver;
    return success;
}

bool UserService::updateDriverLocation(int driverId, int x, int y) {
    Driver* existingDriver = driverDAO->getById(driverId);
    if (!existingDriver) {
        Logger::getInstance()->error("Failed to update driver location: driver not found");
        return false;
    }
    
    // 只有在线或可用状态的司机才能更新位置
    if (existingDriver->getStatus() != DriverStatus::ONLINE && 
        existingDriver->getStatus() != DriverStatus::AVAILABLE) {
        delete existingDriver;
        Logger::getInstance()->error("Failed to update driver location: driver is not online");
        return false;
    }
    
    Location oldLocation = existingDriver->getLocation();
    bool locationChanged = (oldLocation.x != x || oldLocation.y != y);
    
    bool success = driverDAO->updateLocation(driverId, x, y);
    
    if (success && locationChanged) {
        // 更新空间索引
        if (existingDriver->getStatus() == DriverStatus::AVAILABLE) {
            spatialIndex->updateDriver(driverId, x, y);
            // 触发匹配
            matchingService->onDriverUpdate(driverId, true);
        }
        
        Logger::getInstance()->debug("Updated driver " + std::to_string(driverId) + 
                                  " location to (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    }
    
    delete existingDriver;
    return success;
}

bool UserService::updateDriverRating(int driverId, float newRating) {
    if (newRating < 1.0f || newRating > 5.0f) {
        Logger::getInstance()->error("Failed to update driver rating: rating must be between 1.0 and 5.0");
        return false;
    }
    
    bool success = driverDAO->updateRating(driverId, newRating);
    
    if (success) {
        Logger::getInstance()->info("Updated driver rating for id " + std::to_string(driverId) + 
                                  " to " + std::to_string(newRating));
    }
    
    return success;
}

bool UserService::deleteDriver(int driverId) {
    // 检查是否有未完成的行程
    std::vector<Trip*> activeTrips = tripDAO->getActiveTripsByDriver(driverId);
    if (!activeTrips.empty()) {
        for (Trip* trip : activeTrips) {
            delete trip;
        }
        Logger::getInstance()->error("Failed to delete driver: has active trips");
        return false;
    }
    
    // 从空间索引移除
    spatialIndex->removeDriver(driverId);
    
    bool success = driverDAO->deleteById(driverId);
    
    if (success) {
        Logger::getInstance()->info("Deleted driver with id: " + std::to_string(driverId));
    } else {
        Logger::getInstance()->error("Failed to delete driver from database");
    }
    
    return success;
}

// 出行请求相关方法实现
RideRequest* UserService::createRideRequest(int riderId, const Location& startLocation, 
                                          const Location& endLocation, time_t earliestDeparture, 
                                          time_t latestDeparture) {
    // 验证乘客存在
    Rider* rider = riderDAO->getById(riderId);
    if (!rider) {
        Logger::getInstance()->error("Failed to create ride request: rider not found");
        return nullptr;
    }
    delete rider;
    
    // 验证时间窗口
    time_t now = time(nullptr);
    if (earliestDeparture < now) {
        Logger::getInstance()->error("Failed to create ride request: earliest departure time in the past");
        return nullptr;
    }
    if (latestDeparture <= earliestDeparture) {
        Logger::getInstance()->error("Failed to create ride request: latest departure time must be after earliest");
        return nullptr;
    }
    
    RideRequest request;
    request.setRiderId(riderId);
    request.setStartLocation(startLocation);
    request.setEndLocation(endLocation);
    request.setEarliestDepartureTime(earliestDeparture);
    request.setLatestDepartureTime(latestDeparture);
    request.setStatus(RideRequestStatus::PENDING);
    request.setCreationTime(now);
    
    int requestId = rideRequestDAO->create(request);
    if (requestId <= 0) {
        Logger::getInstance()->error("Failed to create ride request in database");
        return nullptr;
    }
    
    // 更新统计信息
    statsMonitor->incrementPendingRequests();
    
    // 触发匹配
    matchingService->matchRequest(requestId);
    
    Logger::getInstance()->info("Created new ride request with id: " + std::to_string(requestId) + 
                              " for rider: " + std::to_string(riderId));
    
    return rideRequestDAO->getById(requestId);
}

RideRequest* UserService::getRideRequestById(int requestId) {
    return rideRequestDAO->getById(requestId);
}

bool UserService::cancelRideRequest(int requestId) {
    RideRequest* request = rideRequestDAO->getById(requestId);
    if (!request) {
        Logger::getInstance()->error("Failed to cancel ride request: request not found");
        return false;
    }
    
    // 只有待匹配的请求才能取消
    if (request->getStatus() != RideRequestStatus::PENDING) {
        delete request;
        Logger::getInstance()->error("Failed to cancel ride request: request is not in pending status");
        return false;
    }
    
    bool success = rideRequestDAO->updateStatus(requestId, RideRequestStatus::CANCELLED);
    
    if (success) {
        // 更新统计信息
        statsMonitor->decrementPendingRequests();
        Logger::getInstance()->info("Cancelled ride request with id: " + std::to_string(requestId));
    }
    
    delete request;
    return success;
}

// 行程相关方法实现
Trip* UserService::getTripById(int tripId) {
    return tripDAO->getById(tripId);
}

std::vector<Trip*> UserService::getTripsByDriver(int driverId) {
    return tripDAO->getByDriverId(driverId);
}

std::vector<Trip*> UserService::getTripsByRider(int riderId) {
    return tripDAO->getByRiderId(riderId);
}

bool UserService::startTrip(int tripId) {
    Trip* trip = tripDAO->getById(tripId);
    if (!trip) {
        Logger::getInstance()->error("Failed to start trip: trip not found");
        return false;
    }
    
    if (trip->getStatus() != TripStatus::ONGOING) {
        delete trip;
        Logger::getInstance()->error("Failed to start trip: trip is not in ongoing status");
        return false;
    }
    
    // 设置行程开始时间
    trip->setStartTime(time(nullptr));
    
    bool success = tripDAO->update(*trip);
    
    if (success) {
        Logger::getInstance()->info("Started trip with id: " + std::to_string(tripId));
    }
    
    delete trip;
    return success;
}

bool UserService::completeTrip(int tripId, float driverRating, float riderRating) {
    std::lock_guard<std::mutex> lock(matchMutex);
    
    Trip* trip = tripDAO->getById(tripId);
    if (!trip) {
        Logger::getInstance()->error("Failed to complete trip: trip not found");
        return false;
    }
    
    if (trip->getStatus() != TripStatus::ONGOING) {
        delete trip;
        Logger::getInstance()->error("Failed to complete trip: trip is not in ongoing status");
        return false;
    }
    
    if (driverRating < 1.0f || driverRating > 5.0f || riderRating < 1.0f || riderRating > 5.0f) {
        delete trip;
        Logger::getInstance()->error("Failed to complete trip: invalid ratings");
        return false;
    }
    
    // 更新行程状态
    trip->setStatus(TripStatus::COMPLETED);
    trip->setEndTime(time(nullptr));
    trip->setDriverRating(driverRating);
    trip->setRiderRating(riderRating);
    
    // 事务处理
    bool success = tripDAO->update(*trip);
    
    if (success) {
        // 更新司机评分和状态
        updateDriverRating(trip->getDriverId(), driverRating);
        updateDriverStatus(trip->getDriverId(), DriverStatus::AVAILABLE);
        
        // 更新乘客评分
        updateRiderRating(trip->getRiderId(), riderRating);
        
        // 更新统计信息
        statsMonitor->decrementActiveTrips();
        statsMonitor->incrementAvailableDrivers();
        
        Logger::getInstance()->info("Completed trip with id: " + std::to_string(tripId) + 
                                  " between rider " + std::to_string(trip->getRiderId()) + 
                                  " and driver " + std::to_string(trip->getDriverId()));
    }
    
    delete trip;
    return success;
}

bool UserService::cancelTrip(int tripId) {
    std::lock_guard<std::mutex> lock(matchMutex);
    
    Trip* trip = tripDAO->getById(tripId);
    if (!trip) {
        Logger::getInstance()->error("Failed to cancel trip: trip not found");
        return false;
    }
    
    if (trip->getStatus() != TripStatus::ONGOING) {
        delete trip;
        Logger::getInstance()->error("Failed to cancel trip: trip is not in ongoing status");
        return false;
    }
    
    // 更新行程状态
    trip->setStatus(TripStatus::CANCELLED);
    trip->setEndTime(time(nullptr));
    
    bool success = tripDAO->update(*trip);
    
    if (success) {
        // 更新司机状态为可用
        updateDriverStatus(trip->getDriverId(), DriverStatus::AVAILABLE);
        
        // 更新对应的请求状态
        RideRequest* request = rideRequestDAO->getById(trip->getRequestId());
        if (request && request->getStatus() == RideRequestStatus::MATCHED) {
            rideRequestDAO->updateStatus(request->getId(), RideRequestStatus::CANCELLED);
            delete request;
        }
        
        // 更新统计信息
        statsMonitor->decrementActiveTrips();
        statsMonitor->incrementAvailableDrivers();
        
        Logger::getInstance()->info("Cancelled trip with id: " + std::to_string(tripId));
    }
    
    delete trip;
    return success;
}

// 批量获取方法
std::vector<Rider*> UserService::getAllRiders() {
    return riderDAO->getAll();
}

std::vector<Driver*> UserService::getAllDrivers() {
    return driverDAO->getAll();
}

std::vector<Driver*> UserService::getAvailableDrivers() {
    return driverDAO->getAvailableDrivers();
}
