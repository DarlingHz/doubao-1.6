// 空间索引，用于优化地理位置查询
#ifndef SPATIAL_INDEX_H
#define SPATIAL_INDEX_H

#include "../model/entities.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <memory>
#include <cmath>

// 网格索引的单元格大小
const int GRID_CELL_SIZE = 10;

// 网格键结构，用于表示一个网格单元格
struct GridKey {
    int gridX;
    int gridY;
    
    GridKey(int x, int y) : gridX(x), gridY(y) {}
    
    bool operator==(const GridKey& other) const {
        return gridX == other.gridX && gridY == other.gridY;
    }
};

// 为GridKey特化哈希函数
namespace std {
    template<>
    struct hash<GridKey> {
        size_t operator()(const GridKey& key) const {
            // 简单的哈希组合方法
            return (hash<int>()(key.gridX) << 1) ^ hash<int>()(key.gridY);
        }
    };
}

// 空间索引类，使用网格来组织可用车主
class SpatialIndex {
private:
    // 网格到车主ID列表的映射
    std::unordered_map<GridKey, std::vector<int>> gridMap;
    
    // 车主ID到位置的快速查找映射
    std::unordered_map<int, Location> driverLocations;
    
    // 互斥锁，保护索引的并发访问
    std::mutex indexMutex;
    
    // 将坐标转换为网格键
    GridKey getGridKey(const Location& location) const {
        int gridX = std::floor(location.x / (float)GRID_CELL_SIZE);
        int gridY = std::floor(location.y / (float)GRID_CELL_SIZE);
        return GridKey(gridX, gridY);
    }
    
    // 获取指定位置周围的网格键
    std::vector<GridKey> getSurroundingGridKeys(const Location& location, int radius) const {
        std::vector<GridKey> keys;
        
        // 计算需要查询的网格范围
        int gridRadius = std::ceil(radius / (float)GRID_CELL_SIZE);
        GridKey centerKey = getGridKey(location);
        
        // 添加周围的网格键
        for (int dx = -gridRadius; dx <= gridRadius; dx++) {
            for (int dy = -gridRadius; dy <= gridRadius; dy++) {
                keys.emplace_back(centerKey.gridX + dx, centerKey.gridY + dy);
            }
        }
        
        return keys;
    }
    
public:
    SpatialIndex();
    ~SpatialIndex();
    
    // 添加车主到索引
    void addDriver(int driverId, const Location& location);
    
    // 更新车主位置
    void updateDriverLocation(int driverId, const Location& newLocation);
    
    // 从索引中移除车主
    void removeDriver(int driverId);
    
    // 查询指定位置附近的车主ID列表
    std::vector<int> queryNearbyDrivers(const Location& location, int radius);
    
    // 获取索引中的车主总数
    size_t getDriverCount();
    
    // 清空索引
    void clear();
};

#endif // SPATIAL_INDEX_H
