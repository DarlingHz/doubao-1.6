// 空间索引实现
#include "spatial_index.h"

SpatialIndex::SpatialIndex() {
}

SpatialIndex::~SpatialIndex() {
}

void SpatialIndex::addDriver(int driverId, const Location& location) {
    std::lock_guard<std::mutex> lock(indexMutex);
    
    // 检查司机是否已经在索引中
    auto locIt = driverLocations.find(driverId);
    if (locIt != driverLocations.end()) {
        // 如果司机已经在索引中，先移除旧位置
        removeDriver(driverId);
    }
    
    // 添加到新位置
    GridKey key = getGridKey(location);
    gridMap[key].push_back(driverId);
    driverLocations[driverId] = location;
}

void SpatialIndex::updateDriverLocation(int driverId, const Location& newLocation) {
    std::lock_guard<std::mutex> lock(indexMutex);
    
    // 检查司机是否在索引中
    auto locIt = driverLocations.find(driverId);
    if (locIt == driverLocations.end()) {
        // 如果司机不在索引中，直接添加
        addDriver(driverId, newLocation);
        return;
    }
    
    // 检查位置是否改变
    if (locIt->second.x == newLocation.x && locIt->second.y == newLocation.y) {
        // 位置没有改变，无需更新
        return;
    }
    
    // 获取旧的网格键并从旧网格中移除
    GridKey oldKey = getGridKey(locIt->second);
    auto gridIt = gridMap.find(oldKey);
    if (gridIt != gridMap.end()) {
        auto& driverList = gridIt->second;
        auto driverIt = std::find(driverList.begin(), driverList.end(), driverId);
        if (driverIt != driverList.end()) {
            driverList.erase(driverIt);
        }
        
        // 如果网格为空，移除该网格
        if (driverList.empty()) {
            gridMap.erase(gridIt);
        }
    }
    
    // 添加到新的网格
    GridKey newKey = getGridKey(newLocation);
    gridMap[newKey].push_back(driverId);
    
    // 更新位置映射
    locIt->second = newLocation;
}

void SpatialIndex::removeDriver(int driverId) {
    std::lock_guard<std::mutex> lock(indexMutex);
    
    // 查找司机的位置
    auto locIt = driverLocations.find(driverId);
    if (locIt == driverLocations.end()) {
        return;  // 司机不在索引中
    }
    
    // 获取网格键并从网格中移除
    GridKey key = getGridKey(locIt->second);
    auto gridIt = gridMap.find(key);
    if (gridIt != gridMap.end()) {
        auto& driverList = gridIt->second;
        auto driverIt = std::find(driverList.begin(), driverList.end(), driverId);
        if (driverIt != driverList.end()) {
            driverList.erase(driverIt);
        }
        
        // 如果网格为空，移除该网格
        if (driverList.empty()) {
            gridMap.erase(gridIt);
        }
    }
    
    // 从位置映射中移除
    driverLocations.erase(locIt);
}

std::vector<int> SpatialIndex::queryNearbyDrivers(const Location& location, int radius) {
    std::lock_guard<std::mutex> lock(indexMutex);
    std::vector<int> nearbyDrivers;
    
    // 获取需要查询的网格键
    std::vector<GridKey> gridKeys = getSurroundingGridKeys(location, radius);
    
    // 用于记录已经找到的司机，避免重复
    std::unordered_map<int, bool> seenDrivers;
    
    // 检查每个网格中的司机
    for (const auto& key : gridKeys) {
        auto gridIt = gridMap.find(key);
        if (gridIt != gridMap.end()) {
            for (int driverId : gridIt->second) {
                // 避免重复添加
                if (seenDrivers.find(driverId) != seenDrivers.end()) {
                    continue;
                }
                
                // 检查司机是否在指定半径内
                auto locIt = driverLocations.find(driverId);
                if (locIt != driverLocations.end()) {
                    int distance = locIt->second.manhattanDistance(location);
                    if (distance <= radius) {
                        nearbyDrivers.push_back(driverId);
                        seenDrivers[driverId] = true;
                    }
                }
            }
        }
    }
    
    return nearbyDrivers;
}

size_t SpatialIndex::getDriverCount() {
    std::lock_guard<std::mutex> lock(indexMutex);
    return driverLocations.size();
}

void SpatialIndex::clear() {
    std::lock_guard<std::mutex> lock(indexMutex);
    gridMap.clear();
    driverLocations.clear();
}
