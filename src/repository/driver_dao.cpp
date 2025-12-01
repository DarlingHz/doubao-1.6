// 车主数据访问对象实现
#include "driver_dao.h"
#include "db_connection.h"
#include <sstream>
#include <iomanip>

// 回调函数数据结构
struct DriverCallbackData {
    std::unique_ptr<Driver>* driver;
    std::vector<std::unique_ptr<Driver>>* driverList;
};

DriverDAO::DriverDAO() {
    dbManager = DBManager::getInstance();
}

DriverDAO::~DriverDAO() {
}

std::string DriverDAO::timePointToString(const TimePoint& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

TimePoint DriverDAO::stringToTimePoint(const std::string& timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

int DriverDAO::driverStatusToInt(DriverStatus status) {
    switch (status) {
        case DriverStatus::OFFLINE:
            return 0;
        case DriverStatus::AVAILABLE:
            return 1;
        case DriverStatus::ON_TRIP:
            return 2;
        default:
            return 0;
    }
}

DriverStatus DriverDAO::intToDriverStatus(int statusInt) {
    switch (statusInt) {
        case 0:
            return DriverStatus::OFFLINE;
        case 1:
            return DriverStatus::AVAILABLE;
        case 2:
            return DriverStatus::ON_TRIP;
        default:
            return DriverStatus::OFFLINE;
    }
}

// 回调函数
static int driverCallback(void* data, int argc, char** argv, char** colName) {
    DriverCallbackData* callbackData = static_cast<DriverCallbackData*>(data);
    
    int id = std::stoi(argv[0]);
    std::string name = argv[1];
    std::string licensePlate = argv[2];
    std::string carModel = argv[3];
    int capacity = std::stoi(argv[4]);
    DriverStatus status = DriverStatus::OFFLINE;
    float rating = 5.0f;
    Location location;
    int activeTripsCount = 0;
    
    if (argv[5]) status = static_cast<DriverDAO*>(callbackData)->intToDriverStatus(std::stoi(argv[5]));
    if (argv[6]) rating = std::stof(argv[6]);
    if (argv[7]) location.x = std::stoi(argv[7]);
    if (argv[8]) location.y = std::stoi(argv[8]);
    if (argv[10]) activeTripsCount = std::stoi(argv[10]);
    
    auto driver = std::make_unique<Driver>(id, name, licensePlate, carModel, capacity);
    driver->setStatus(status);
    driver->setRating(rating);
    driver->setCurrentLocation(location);
    
    // 设置活跃行程数
    for (int i = 0; i < activeTripsCount; i++) {
        driver->incrementActiveTrips();
    }
    
    if (callbackData->driver) {
        *callbackData->driver = std::move(driver);
    } else if (callbackData->driverList) {
        callbackData->driverList->push_back(std::move(driver));
    }
    
    return 0;
}

int DriverDAO::createDriver(const std::string& name, const std::string& licensePlate, 
                           const std::string& carModel, int capacity) {
    DBConnection* connection = dbManager->getConnection();
    std::lock_guard<std::mutex> lock(connection->getMutex());
    
    std::stringstream sql;
    sql << "INSERT INTO drivers (name, license_plate, car_model, capacity, registration_time) VALUES (\""
        << name << \", \"" << licensePlate << \", \"" << carModel << \", "
        << capacity << ", \"" << timePointToString(std::chrono::system_clock::now()) << \");"
        << " SELECT last_insert_rowid();";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return -1;
    }
    
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return id;
}

std::unique_ptr<Driver> DriverDAO::getDriverById(int id) {
    DBConnection* connection = dbManager->getConnection();
    std::unique_ptr<Driver> driver;
    DriverCallbackData callbackData = {&driver, nullptr};
    
    std::stringstream sql;
    sql << "SELECT id, name, license_plate, car_model, capacity, status, rating, "
        << "current_x, current_y, registration_time, active_trips_count FROM drivers WHERE id = " << id;
    
    // 这里需要特殊处理回调函数，因为回调函数需要访问intToDriverStatus方法
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return nullptr;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int driverId = sqlite3_column_int(stmt, 0);
        std::string name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::string licensePlate(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        std::string carModel(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        int capacity = sqlite3_column_int(stmt, 4);
        DriverStatus status = intToDriverStatus(sqlite3_column_int(stmt, 5));
        float rating = sqlite3_column_double(stmt, 6);
        int x = sqlite3_column_int(stmt, 7);
        int y = sqlite3_column_int(stmt, 8);
        int activeTripsCount = sqlite3_column_int(stmt, 10);
        
        driver = std::make_unique<Driver>(driverId, name, licensePlate, carModel, capacity);
        driver->setStatus(status);
        driver->setRating(rating);
        driver->setCurrentLocation(Location(x, y));
        
        // 设置活跃行程数
        for (int i = 0; i < activeTripsCount; i++) {
            driver->incrementActiveTrips();
        }
    }
    
    sqlite3_finalize(stmt);
    return driver;
}

bool DriverDAO::updateDriver(const Driver& driver) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE drivers SET name = \"" << driver.getName() 
        << \", license_plate = \"" << driver.getLicensePlate()
        << \", car_model = \"" << driver.getCarModel()
        << \", capacity = " << driver.getCapacity()
        << \", status = " << driverStatusToInt(driver.getStatus())
        << \", rating = " << driver.getRating()
        << \", current_x = " << driver.getCurrentLocation().x
        << \", current_y = " << driver.getCurrentLocation().y
        << \", active_trips_count = " << driver.getActiveTripsCount()
        << " WHERE id = " << driver.getId();
    
    return connection->execute(sql.str());
}

bool DriverDAO::updateDriverStatus(int id, DriverStatus status) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE drivers SET status = " << driverStatusToInt(status) << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

bool DriverDAO::updateDriverLocation(int id, const Location& location) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE drivers SET current_x = " << location.x 
        << ", current_y = " << location.y 
        << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

bool DriverDAO::updateDriverRating(int id, float rating) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE drivers SET rating = " << rating << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

std::vector<std::unique_ptr<Driver>> DriverDAO::getAvailableDrivers() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Driver>> driverList;
    
    std::stringstream sql;
    sql << "SELECT id, name, license_plate, car_model, capacity, status, rating, "
        << "current_x, current_y, registration_time, active_trips_count FROM drivers WHERE status = " 
        << driverStatusToInt(DriverStatus::AVAILABLE);
    
    // 这里使用预处理语句来处理结果
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return driverList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int driverId = sqlite3_column_int(stmt, 0);
        std::string name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::string licensePlate(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        std::string carModel(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        int capacity = sqlite3_column_int(stmt, 4);
        DriverStatus status = intToDriverStatus(sqlite3_column_int(stmt, 5));
        float rating = sqlite3_column_double(stmt, 6);
        int x = sqlite3_column_int(stmt, 7);
        int y = sqlite3_column_int(stmt, 8);
        int activeTripsCount = sqlite3_column_int(stmt, 10);
        
        auto driver = std::make_unique<Driver>(driverId, name, licensePlate, carModel, capacity);
        driver->setStatus(status);
        driver->setRating(rating);
        driver->setCurrentLocation(Location(x, y));
        
        // 设置活跃行程数
        for (int i = 0; i < activeTripsCount; i++) {
            driver->incrementActiveTrips();
        }
        
        driverList.push_back(std::move(driver));
    }
    
    sqlite3_finalize(stmt);
    return driverList;
}

std::vector<std::unique_ptr<Driver>> DriverDAO::getAvailableDriversInRange(const Location& center, int maxDistance) {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Driver>> driverList;
    
    // 使用曼哈顿距离来过滤
    int minX = center.x - maxDistance;
    int maxX = center.x + maxDistance;
    int minY = center.y - maxDistance;
    int maxY = center.y + maxDistance;
    
    std::stringstream sql;
    sql << "SELECT id, name, license_plate, car_model, capacity, status, rating, "
        << "current_x, current_y, registration_time, active_trips_count FROM drivers "
        << "WHERE status = " << driverStatusToInt(DriverStatus::AVAILABLE) << " AND "
        << "current_x BETWEEN " << minX << " AND " << maxX << " AND "
        << "current_y BETWEEN " << minY << " AND " << maxY;
    
    // 使用预处理语句来处理结果
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return driverList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int driverId = sqlite3_column_int(stmt, 0);
        std::string name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::string licensePlate(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        std::string carModel(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        int capacity = sqlite3_column_int(stmt, 4);
        DriverStatus status = intToDriverStatus(sqlite3_column_int(stmt, 5));
        float rating = sqlite3_column_double(stmt, 6);
        int x = sqlite3_column_int(stmt, 7);
        int y = sqlite3_column_int(stmt, 8);
        int activeTripsCount = sqlite3_column_int(stmt, 10);
        
        // 再次确认曼哈顿距离在限制内
        Location driverLoc(x, y);
        if (driverLoc.manhattanDistance(center) <= maxDistance) {
            auto driver = std::make_unique<Driver>(driverId, name, licensePlate, carModel, capacity);
            driver->setStatus(status);
            driver->setRating(rating);
            driver->setCurrentLocation(driverLoc);
            
            // 设置活跃行程数
            for (int i = 0; i < activeTripsCount; i++) {
                driver->incrementActiveTrips();
            }
            
            driverList.push_back(std::move(driver));
        }
    }
    
    sqlite3_finalize(stmt);
    return driverList;
}

bool DriverDAO::updateActiveTripsCount(int id, int count) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "UPDATE drivers SET active_trips_count = " << count << " WHERE id = " << id;
    
    return connection->execute(sql.str());
}

std::vector<std::unique_ptr<Driver>> DriverDAO::getAllDrivers() {
    DBConnection* connection = dbManager->getConnection();
    std::vector<std::unique_ptr<Driver>> driverList;
    
    std::stringstream sql;
    sql << "SELECT id, name, license_plate, car_model, capacity, status, rating, "
        << "current_x, current_y, registration_time, active_trips_count FROM drivers";
    
    // 使用预处理语句来处理结果
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(connection->getDB(), sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(connection->getDB()) << std::endl;
        return driverList;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int driverId = sqlite3_column_int(stmt, 0);
        std::string name(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        std::string licensePlate(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        std::string carModel(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        int capacity = sqlite3_column_int(stmt, 4);
        DriverStatus status = intToDriverStatus(sqlite3_column_int(stmt, 5));
        float rating = sqlite3_column_double(stmt, 6);
        int x = sqlite3_column_int(stmt, 7);
        int y = sqlite3_column_int(stmt, 8);
        int activeTripsCount = sqlite3_column_int(stmt, 10);
        
        auto driver = std::make_unique<Driver>(driverId, name, licensePlate, carModel, capacity);
        driver->setStatus(status);
        driver->setRating(rating);
        driver->setCurrentLocation(Location(x, y));
        
        // 设置活跃行程数
        for (int i = 0; i < activeTripsCount; i++) {
            driver->incrementActiveTrips();
        }
        
        driverList.push_back(std::move(driver));
    }
    
    sqlite3_finalize(stmt);
    return driverList;
}

bool DriverDAO::deleteDriver(int id) {
    DBConnection* connection = dbManager->getConnection();
    
    std::stringstream sql;
    sql << "DELETE FROM drivers WHERE id = " << id;
    
    return connection->execute(sql.str());
}
