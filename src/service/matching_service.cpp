// 匹配服务实现
#include "matching_service.h"
#include "../model/entities.h"
#include "../repository/ride_request_dao.h"
#include "../repository/driver_dao.h"
#include "../repository/trip_dao.h"
#include "../utils/spatial_index.h"
#include "../utils/logger.h"
#include "../utils/stats_monitor.h"
#include <thread>
#include <chrono>
#include <algorithm>

MatchingService* MatchingService::instance = nullptr;

MatchingService::MatchingService() :
    stopRequested(false),
    maxMatchDistance(5000),
    gridSize(1000) {
    // 初始化依赖
    rideRequestDAO = std::make_unique<RideRequestDAO>();
    driverDAO = std::make_unique<DriverDAO>();
    tripDAO = std::make_unique<TripDAO>();
    spatialIndex = std::make_unique<SpatialIndex>(gridSize);
    statsMonitor = StatsMonitor::getInstance();
}

MatchingService* MatchingService::getInstance() {
    static std::mutex mutex;
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new MatchingService();
        }
    }
    return instance;
}

void MatchingService::initialize(int maxDistance, int gridSize) {
    this->maxMatchDistance = maxDistance;
    this->gridSize = gridSize;
    spatialIndex = std::make_unique<SpatialIndex>(gridSize);
    
    Logger::getInstance()->info("MatchingService initialized with max distance: " + 
                              std::to_string(maxDistance) + ", grid size: " + 
                              std::to_string(gridSize));
}

void MatchingService::start() {
    stopRequested = false;
    std::thread matchingThread(&MatchingService::matchingThreadFunction, this);
    matchingThread.detach();
    
    Logger::getInstance()->info("Matching service started");
}

void MatchingService::stop() {
    stopRequested = true;
    matchCondition.notify_one();
    Logger::getInstance()->info("Matching service stopped");
}

void MatchingService::triggerMatching() {
    matchCondition.notify_one();
}

bool MatchingService::matchRequest(int requestId) {
    try {
        RideRequest* request = rideRequestDAO->getById(requestId);
        if (!request || request->getStatus() != RideRequestStatus::PENDING) {
            delete request;
            return false;
        }
        
        bool matched = tryMatchRequest(*request);
        delete request;
        return matched;
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error matching request " + std::to_string(requestId) + ": " + e.what());
        return false;
    }
}

void MatchingService::onDriverUpdate(int driverId, bool locationChanged) {
    try {
        Driver* driver = driverDAO->getById(driverId);
        if (!driver) {
            return;
        }
        
        if (driver->getStatus() == DriverStatus::AVAILABLE && locationChanged) {
            // 更新空间索引
            spatialIndex->updateDriver(driverId, driver->getLocation().x, driver->getLocation().y);
        } else if (driver->getStatus() == DriverStatus::OFFLINE) {
            // 从索引中移除
            spatialIndex->removeDriver(driverId);
        }
        
        delete driver;
        
        // 如果司机变为可用状态，触发匹配
        if (driver->getStatus() == DriverStatus::AVAILABLE) {
            triggerMatching();
        }
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Error handling driver update for " + std::to_string(driverId) + ": " + e.what());
    }
}

int MatchingService::getPendingRequestsCount() {
    return rideRequestDAO->getPendingRequestsCount();
}

void MatchingService::matchingThreadFunction() {
    while (!stopRequested) {
        std::unique_lock<std::mutex> lock(matchMutex);
        
        // 等待匹配触发或超时
        auto result = matchCondition.wait_for(lock, std::chrono::seconds(5), 
                                           [this] { return stopRequested || getPendingRequestsCount() > 0; });
        
        if (stopRequested) {
            break;
        }
        
        if (!result) {
            // 超时，定期检查是否有待处理请求
            continue;
        }
        
        // 获取所有待处理请求
        std::vector<RideRequest*> pendingRequests = rideRequestDAO->getPendingRequests();
        
        // 尝试匹配每个请求
        for (auto request : pendingRequests) {
            tryMatchRequest(*request);
            delete request;
        }
    }
}

int MatchingService::calculateManhattanDistance(int startX, int startY, int endX, int endY) {
    return std::abs(startX - endX) + std::abs(startY - endY);
}

bool MatchingService::tryMatchRequest(const RideRequest& request) {
    // 计时开始
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 检查请求是否仍处于待匹配状态
    RideRequest* currentRequest = rideRequestDAO->getById(request.getId());
    if (!currentRequest || currentRequest->getStatus() != RideRequestStatus::PENDING) {
        delete currentRequest;
        return false;
    }
    delete currentRequest;
    
    // 从空间索引中获取附近的司机
    std::vector<int> nearbyDriverIds = spatialIndex->findNearbyDrivers(
        request.getStartLocation().x, request.getStartLocation().y, maxMatchDistance);
    
    if (nearbyDriverIds.empty()) {
        Logger::getInstance()->debug("No nearby drivers found for request " + std::to_string(request.getId()));
        return false;
    }
    
    // 获取司机详细信息
    std::vector<Driver*> nearbyDrivers;
    for (int driverId : nearbyDriverIds) {
        Driver* driver = driverDAO->getById(driverId);
        if (driver && driver->getStatus() == DriverStatus::AVAILABLE) {
            nearbyDrivers.push_back(driver);
        }
    }
    
    if (nearbyDrivers.empty()) {
        // 清理内存
        for (auto driver : nearbyDrivers) {
            delete driver;
        }
        Logger::getInstance()->debug("No available drivers found for request " + std::to_string(request.getId()));
        return false;
    }
    
    // 寻找最佳匹配的司机
    Driver* bestDriver = findBestDriver(request, nearbyDrivers);
    
    // 清理不需要的司机对象
    for (auto driver : nearbyDrivers) {
        if (driver != bestDriver) {
            delete driver;
        }
    }
    
    if (!bestDriver) {
        return false;
    }
    
    // 处理匹配成功
    bool success = handleMatchSuccess(request, bestDriver);
    delete bestDriver;
    
    // 计算匹配耗时
    auto endTime = std::chrono::high_resolution_clock::now();
    long matchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // 记录性能统计
    if (success) {
        int distance = calculateManhattanDistance(
            request.getStartLocation().x, request.getStartLocation().y,
            bestDriver->getLocation().x, bestDriver->getLocation().y);
        statsMonitor->recordMatchSuccess(matchTimeMs, distance);
    }
    
    return success;
}

Driver* MatchingService::findBestDriver(const RideRequest& request, const std::vector<Driver*>& nearbyDrivers) {
    Driver* bestDriver = nullptr;
    int minDistance = INT_MAX;
    int minTripCount = INT_MAX;
    time_t earliestRegistration = time(nullptr);
    
    for (Driver* driver : nearbyDrivers) {
        // 检查时间窗口是否匹配
        if (!rideRequestDAO->isTimeWindowValid(request.getId(), driver->getId())) {
            continue;
        }
        
        // 计算距离
        int distance = calculateManhattanDistance(
            request.getStartLocation().x, request.getStartLocation().y,
            driver->getLocation().x, driver->getLocation().y);
        
        // 获取司机的当前行程数
        int tripCount = tripDAO->getActiveTripCountByDriver(driver->getId());
        
        // 选择最优司机的策略：
        // 1. 距离最近
        // 2. 若距离相同，选择行程数最少的
        // 3. 若行程数相同，选择注册时间最早的
        bool isBetterMatch = false;
        if (distance < minDistance) {
            isBetterMatch = true;
        } else if (distance == minDistance) {
            if (tripCount < minTripCount) {
                isBetterMatch = true;
            } else if (tripCount == minTripCount && driver->getRegistrationTime() < earliestRegistration) {
                isBetterMatch = true;
            }
        }
        
        if (isBetterMatch) {
            bestDriver = driver;
            minDistance = distance;
            minTripCount = tripCount;
            earliestRegistration = driver->getRegistrationTime();
        }
    }
    
    if (bestDriver) {
        Logger::getInstance()->debug("Best driver " + std::to_string(bestDriver->getId()) + 
                                  " found for request " + std::to_string(request.getId()) + 
                                  " with distance " + std::to_string(minDistance));
    }
    
    return bestDriver;
}

bool MatchingService::handleMatchSuccess(const RideRequest& request, Driver* driver) {
    std::lock_guard<std::mutex> lock(matchMutex);
    
    // 事务开始：更新请求状态、司机状态、创建行程
    try {
        // 1. 更新司机状态为 ON_TRIP
        if (!driverDAO->updateStatus(driver->getId(), DriverStatus::ON_TRIP)) {
            Logger::getInstance()->error("Failed to update driver status for match");
            return false;
        }
        
        // 2. 更新请求状态为 MATCHED
        if (!rideRequestDAO->updateStatus(request.getId(), RideRequestStatus::MATCHED)) {
            Logger::getInstance()->error("Failed to update ride request status for match");
            // 回滚司机状态
            driverDAO->updateStatus(driver->getId(), DriverStatus::AVAILABLE);
            return false;
        }
        
        // 3. 创建行程记录
        Trip trip;
        trip.setDriverId(driver->getId());
        trip.setRiderId(request.getRiderId());
        trip.setRequestId(request.getId());
        trip.setMatchTime(time(nullptr));
        trip.setStatus(TripStatus::ONGOING);
        trip.setStartLocation(request.getStartLocation());
        trip.setEndLocation(request.getEndLocation());
        
        int tripId = tripDAO->create(trip);
        if (tripId <= 0) {
            Logger::getInstance()->error("Failed to create trip record");
            // 回滚
            driverDAO->updateStatus(driver->getId(), DriverStatus::AVAILABLE);
            rideRequestDAO->updateStatus(request.getId(), RideRequestStatus::PENDING);
            return false;
        }
        
        // 更新统计信息
        statsMonitor->decrementPendingRequests();
        statsMonitor->decrementAvailableDrivers();
        statsMonitor->incrementActiveTrips();
        
        // 从空间索引中移除司机
        spatialIndex->removeDriver(driver->getId());
        
        Logger::getInstance()->info("Match successful! Trip " + std::to_string(tripId) + 
                                  " created between rider " + std::to_string(request.getRiderId()) + 
                                  " and driver " + std::to_string(driver->getId()));
        
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance()->error("Exception during match handling: " + std::string(e.what()));
        // 尝试回滚
        try {
            driverDAO->updateStatus(driver->getId(), DriverStatus::AVAILABLE);
            rideRequestDAO->updateStatus(request.getId(), RideRequestStatus::PENDING);
        } catch (...) {
            // 忽略回滚错误
        }
        return false;
    }
}
